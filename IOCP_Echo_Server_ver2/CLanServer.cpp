#include "pch.h"
#include "Session.h"
#include "CLanServer.h"
#include "Main.h"

int cnt = 0;

bool CLanServer::Start(const CHAR* openIP, const USHORT port, USHORT createWorkerThreadCount, USHORT maxWorkerThreadCount, ULONG maxClientCount)
{
	InitializeSession(maxClientCount);

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

	// Socket Bind, Listen
	m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_listenSock == INVALID_SOCKET)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return false;
	}

	/*
	* 나중에 lockfree 큐 만들고 나면, tcp 패킷을 큐에 그대로 넣어버리기 위해
	*/
	// SendBuffer 0으로
	/*DWORD sendBufferSize = 0;
	setsockopt(mm_listenSocket, SOL_SOCKET, SO_SNDBUF, (char *)&sendBufferSize, sizeof(sendBufferSize));
	if (optRet == SOCKET_ERROR)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}*/

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);

	int bindRet = bind(m_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return false;
	}

	int listenRet = listen(m_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return false;
	}

	m_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, nullptr, 0, nullptr);
	if (m_hAcceptThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return false;
	}

	// Create IOCP for Release Session
	m_hReleaseCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hReleaseCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return false;
	}

	m_hReleaseThread = (HANDLE)_beginthreadex(NULL, 0, ReleaseThread, m_hReleaseCP, 0, nullptr);
	if (m_hReleaseThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return false;
	}

	// Create IOCP for Network
	m_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hNetworkCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return false;
	}

	// Create WorkerThread
	for (int i = 1; i <= createWorkerThreadCount; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, m_hNetworkCP, 0, nullptr);
		if (hWorkerThread == NULL)
		{
			wprintf(L"ERROR! WorkerThread[%d] running fail..\n", i);
			return false;
		}

		m_WorkerThreads.push_back(hWorkerThread);
		wprintf(L"[SYSTEM] WorkerThread[%d] running..\n", i);
	}

	return true;
}

unsigned int WINAPI CLanServer::AcceptThread(void* arg)
{
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);

	while (1)
	{
		// Accept
		SOCKET client_sock = accept(
			g_Server->m_listenSock, (SOCKADDR*)&clientAddr, &addrlen);

		if (g_bShutdown) break;

		LINGER optval;
		optval.l_onoff = 1;
		optval.l_linger = 0;
		int optRet = setsockopt(client_sock, SOL_SOCKET, SO_LINGER,
			(char*)&optval, sizeof(optval));
		if (optRet == SOCKET_ERROR)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return false;
		}

		if (client_sock == INVALID_SOCKET)
		{
			int err = WSAGetLastError();
			// err 값을 찍어서 정확한 원인을 확인
			printf("accept failed, WSAGetLastError = %d\n", err);
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}

		// maxClientCount를 넘어가면 접속 끊어버리는 코드
		AcquireSRWLockShared(&g_Server->m_usableIdxStackLock);
		bool isFull = g_Server->m_usableIdxStack.empty();
		ReleaseSRWLockShared(&g_Server->m_usableIdxStackLock);

		if (isFull)
		{
			// 세션 최대치 초과 - 접속 거절
			closesocket(client_sock);  // 연결 끊기
			::wprintf(L"[SYSTEM] MaxClientCount 초과! \n");
			continue;  // 다음 클라이언트 대기
		}

		WCHAR clientAddress[16] = { 0 };
		ZeroMemory(clientAddress, sizeof(WCHAR));
		InetNtop(AF_INET, &clientAddr.sin_addr, clientAddress, sizeof(WCHAR));
		if (!g_Server->OnConnectionRequest(clientAddress, ntohs(clientAddr.sin_port)))
		{
			wprintf(L"It's Not Accept IP!!\n");
			continue;
		}

		AcquireSRWLockExclusive(&g_Server->m_usableIdxStackLock);
		ULONG index = g_Server->m_usableIdxStack.back();
		g_Server->m_usableIdxStack.pop_back();
		ReleaseSRWLockExclusive(&g_Server->m_usableIdxStackLock);

		// Create Session
		__int64 sessionId = g_Server->m_sessionIDSupplier++;
		__int64 id = g_Server->CombineIndex(index, sessionId);
		ULONG cindex = g_Server->GetSessionIndex(id);
		__int64 csid = g_Server->GetSessionID(id);

		//Session *pSession = Session::Alloc();
		Session* pSession = new Session(
			id, client_sock, clientAddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		//pSession->Initialize(id, client_sock, clientAddr);
		g_Server->m_pArrSession[index] = pSession;

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->m_ClientSock,
			g_Server->m_hNetworkCP, (ULONG_PTR)pSession, 0);

		g_Server->OnAccept(id);
		//::printf("Accept New Session (ID: %lld)\n", g_Server->GetSessionID(pSession->m_SessionID));

		// recv를 미리 등록해둔다.
		pSession->RecvPost();

	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

// 큐에 들어온 I/O 완료 통지를 실제로 처리하는 스레드
unsigned int WINAPI CLanServer::WorkerThread(void* arg)
{
	HANDLE hNetworkCP = (HANDLE)arg;
	int threadID = GetCurrentThreadId();

	while (1)
	{
		Session* pSession;
		DWORD dwTransferred;
		NetworkOverlapped* pNetOvl = 0;
		//__debugbreak();
		int GQCSRet = GetQueuedCompletionStatus(hNetworkCP, &dwTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown) break;

		if (dwTransferred == 0 && pSession == 0)
		{
			// 정상 종료 루틴
			PostQueuedCompletionStatus(hNetworkCP, 0, 0, 0);
			break;
		}
		// Check Exception
		if (pNetOvl == nullptr)
		{
			if (GQCSRet == FALSE)
			{
				// GetQueuedCompletionStatus Fail
				int err = WSAGetLastError();
				if (err != WSAECONNRESET)
				{
					::printf("Error! %s(%d): %d (thread: %d)\n", __func__, __LINE__, err, threadID);
				}
				PostQueuedCompletionStatus(hNetworkCP, 0, 0, 0);
				break;
			}

		}
		else if (dwTransferred == 0)
		{
			InterlockedExchange(&pSession->m_IsValid, FALSE);
			//continue;
		}
		// Recv
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			pSession->HandleRecvCP(dwTransferred);
			//::printf("%lld: Complete Recv %d bytes (thread: %d)\n", g_Server->GetSessionID(pSession->m_SessionID), dwTransferred, threadID);
		}
		// Send 
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			pSession->HandleSendCP(dwTransferred);
			//::printf("%lld: Complete Send %d bytes (thread: %d)\n", g_Server->GetSessionID(pSession->m_SessionID), dwTransferred, threadID);
		}
		else
		{
			printf("Wrong Type: %d\n", pNetOvl->_type);
		}

		if (InterlockedDecrement(&pSession->m_IOCount) == 0)
		{
			PostQueuedCompletionStatus(
				g_Server->m_hReleaseCP, 0, (ULONG_PTR)pSession, 0);
		}
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

//세션을 삭제하는 스레드
unsigned int WINAPI CLanServer::ReleaseThread(void* arg)
{
	HANDLE hReleaseCP = (HANDLE)arg;

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		OVERLAPPED* pOvl = nullptr;

		int GQCSRet = GetQueuedCompletionStatus(hReleaseCP, &cbTransferred,
			(PULONG_PTR)&pSession, &pOvl, INFINITE);
		if (g_bShutdown) break;

		ULONG index = g_Server->GetSessionIndex(pSession->m_SessionID);

		closesocket(pSession->m_ClientSock);
		__int64 ID = g_Server->GetSessionID(pSession->m_SessionID);
		pSession->Clear();

		EnterCriticalSection(&pSession->_cs);
		LeaveCriticalSection(&pSession->_cs);

		// 락프리메모리풀 추가하고 나면 
		// Session::Free(pSession); 으로 교체
		delete(pSession);

		//::printf("Disconnect Client (ID: %lld) (thread: %d)\n", ID, GetCurrentThreadId());
		AcquireSRWLockExclusive(&g_Server->m_usableIdxStackLock);
		g_Server->m_usableIdxStack.push_back(index);
		ReleaseSRWLockExclusive(&g_Server->m_usableIdxStackLock);

	}
	::printf("Release Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

void CLanServer::SendPacket(const __int64 sessionId, SerializePacket * sPacket)
{
	ULONG idx = GetSessionIndex(sessionId);
	Session* pSession = m_pArrSession[idx];
	pSession->SendPacketQueue(sPacket);
}

void CLanServer::Monitor()
{
	m_AcceptTPS = 0;
	m_RecvMsgTPS = 0;
	m_SendMsgTPS = 0;
}
