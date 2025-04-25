#include "Session.h"
#include "SerializePacket.h"
#include "Main.h"
#include "Define.h"
#include "pch.h"
#include "CLanServer.h"

void Session::HandleRecvCP(int recvBytes)
{
	EnterCriticalSection(&_cs);
	// 링 버퍼 스레드 동기화 문제 없는 지 확인 바람 
	int moveReadRet = m_RecvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_cs);
		return;
	}
	LeaveCriticalSection(&_cs);

	RecvDataToMsg(); // include onMessage, SendMsg, MsgToSendData

	// 데이터 처리 완료했으면 다시 recv 미리 등록해둔다.
	RecvPost();
}

void Session::RecvDataToMsg()
{
	EnterCriticalSection(&_cs);
	int useSize = m_RecvBuf.GetUseSize();

	while (useSize > 0)
	{
		if (useSize <= HEADER_LEN)
			break;

		stHeader header;
		int peekRet = m_RecvBuf.Peek((char*)&header, HEADER_LEN);
		if (peekRet != HEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		if (useSize < HEADER_LEN + header._shLen)
			break;

		int moveReadRet = m_RecvBuf.MoveReadPos(HEADER_LEN);
		if (moveReadRet != HEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		SerializePacket packet;
		int dequeueRet = m_RecvBuf.Dequeue(packet.GetWritePtr(), header._shLen);
		if (dequeueRet != header._shLen)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		packet.MoveWritePos(dequeueRet);
		useSize = m_RecvBuf.GetUseSize();
		LeaveCriticalSection(&_cs);
		g_Server->OnMessage(m_SessionID, &packet);//이러면 g_Server를 무조건 선언해야 되는데..
		EnterCriticalSection(&_cs);
	}

	LeaveCriticalSection(&_cs);
}

//MsgToSendData
void Session::SendPacketQueue(SerializePacket* packet)
{
	EnterCriticalSection(&_cs);
	int packetSize = packet->GetDataSize();
	int enqueueRet = m_SendBuf.Enqueue(
		packet->GetReadPtr(), packetSize);
	if (enqueueRet != packetSize)
	{
		InterlockedDecrement(&m_SendFlag);
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_cs);
		return;
	}

	//::printf("(%s)%d : %d\n", __func__, GetCurrentThreadId(), m_SendFlag);
	if (InterlockedIncrement(&m_SendFlag) == 1)
	{
		LeaveCriticalSection(&_cs);
		SendPost();
	}
	else
	{
		LeaveCriticalSection(&_cs);
	}
}

void Session::HandleSendCP(int sendBytes)
{
	EnterCriticalSection(&_cs);
	int moveReadRet = m_SendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_cs);
		return;
	}
	LeaveCriticalSection(&_cs);

	//보낼 게 아직 남았다는 뜻
	//::printf("(%s)(thread: %d)%d : %d\n", __func__, GetCurrentThreadId(), m_SessionID, m_SendFlag);
	if (InterlockedDecrement(&m_SendFlag) != 0)
	{
		SendPost();
	}
}

// recv I/O 요청해서 IOCP 큐에 완료 통지 들어가게
void Session::RecvPost()
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	EnterCriticalSection(&_cs);
	int freeSize = m_RecvBuf.GetFreeSize();
	_wsaRecvbuf[0].buf = m_RecvBuf.GetWritePtr();
	_wsaRecvbuf[0].len = m_RecvBuf.DirectEnqueueSize();
	_wsaRecvbuf[1].buf = m_RecvBuf.GetBufferFrontPtr();
	_wsaRecvbuf[1].len = freeSize - _wsaRecvbuf[0].len;

	ZeroMemory(&_recvOvl._ovl, sizeof(_recvOvl._ovl)); // 난 이거 생각 못했을 듯... 무조건 해야하나?
	InterlockedIncrement(&m_IOCount);

	_recvOvl._type = NET_TYPE::RECV;
	int recvRet = WSARecv(m_ClientSock, _wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&_recvOvl, NULL);

	//::printf("%lld: Request Recv (thread: %d)\n", g_Server->GetSessionID(m_SessionID), GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			InterlockedDecrement(&m_IOCount);
			LeaveCriticalSection(&_cs);
			return;
		}
	}
	LeaveCriticalSection(&_cs);
}

// send I/O 요청해서 IOCP 큐에 완료 통지 들어가게
void Session::SendPost()
{
	EnterCriticalSection(&_cs);
	DWORD sendBytes;

	int useSize = m_SendBuf.GetUseSize();
	_wsaSendbuf[0].buf = m_SendBuf.GetReadPtr();
	_wsaSendbuf[0].len = m_SendBuf.DirectDequeueSize();
	_wsaSendbuf[1].buf = m_SendBuf.GetBufferFrontPtr();
	_wsaSendbuf[1].len = useSize - _wsaSendbuf[0].len;

	ZeroMemory(&_sendOvl._ovl, sizeof(_sendOvl._ovl));
	InterlockedIncrement(&m_IOCount);
	InterlockedExchange(&m_SendFlag, 1);

	_sendOvl._type = NET_TYPE::SEND;
	int sendRet = WSASend(m_ClientSock, _wsaSendbuf,
		2, &sendBytes, 0, (LPOVERLAPPED)&_sendOvl, NULL);

	//::printf("(thread: %d) %d: Request Send %d bytes\n", GetCurrentThreadId(), m_SessionID, useSize);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			InterlockedDecrement(&m_IOCount);
			LeaveCriticalSection(&_cs);
			return;
		}
	}

	LeaveCriticalSection(&_cs);
}