#pragma once


#define SESSION_INDEX_MASK	0xffff000000000000
#define SESSION_ID_MASK		0x0000ffffffffffff


class CLanServer
{
public:
	bool Start(const CHAR* openIP, const USHORT port, USHORT createWorkerThreadCount, USHORT maxWorkerThreadCount, INT maxClientCount);
	// void Stop();
	inline int GetSessionCount() { return m_ClientCount; }

	void SendPacket(const __int64 sessionId, SerializePacket* sPacket);
	bool Disconnect(Session* pSession);

	void Monitor();

	virtual bool OnConnectionRequest(const WCHAR* ip, USHORT port) = 0;
	virtual void OnAccept(const __int64 sessionID) = 0; // Accept 후 접속 처리 완료 후 호출
	virtual void OnClientLeave(const __int64 sessionID) = 0; // Release 후 호출
	virtual void OnMessage(const __int64 sessionID, SerializePacket* message) = 0; // 패킷 수신 완료 후
	virtual void OnError(int errorcode, WCHAR* errMsg) = 0;

	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI WorkerThread(void* arg);
	static unsigned int WINAPI ReleaseThread(void* arg);

//디버깅 후 private:으로 변경바람
public:
	void InitializeSession(USHORT maxClientCount)
	{
		InitializeSRWLock(&m_usableIdxStackLock);

		// 여기에는 실제 포인터를 저장

		// 스택에 모두 저장
		AcquireSRWLockExclusive(&m_usableIdxStackLock);
		for (int i = maxClientCount - 1; i >= 0; i--)
		{
			m_usableIdxStack.emplace_back(i);
		}
		ReleaseSRWLockExclusive(&m_usableIdxStackLock);
	}
	USHORT GetSessionIndex(__int64 SessionId)
	{
		__int64 p64 = SessionId;
		// 상위 2바이트 꺼내기
		__int64 maskP64 = p64 & SESSION_INDEX_MASK;
		maskP64 = maskP64 >> 48;
		return (USHORT)maskP64;
	}

	__int64 GetSessionID(__int64 SessionId)
	{
		__int64 maskP64 = SessionId & SESSION_ID_MASK;
		return (__int64)maskP64;
	}

	__int64 CombineIndex(USHORT stackIndex, __int64 Index)
	{
		__int64 p64 = Index;
		__int64 index64 = stackIndex;
		index64 = index64 << 48;
		return (p64 | index64);
	}

private:
	//for Completion Port Handle
	HANDLE m_hNetworkCP;
	HANDLE m_hReleaseCP;
	//for thread Handle
	HANDLE m_hReleaseThread;
	HANDLE m_hAcceptThread;
	std::vector<HANDLE> m_WorkerThreads;

	int m_ClientCount = 0;
	__int64 m_sessionIDSupplier = 0;

	SOCKET m_listenSock = INVALID_SOCKET;
	UINT32 m_maxWorkerThreadCount = 0;

	// TPS
	LONG m_AcceptTPS = 0;
	LONG m_RecvMsgTPS = 0;
	LONG m_SendMsgTPS = 0;

	// 최대 Session 수
	USHORT m_MaxSessionCount; // 2바이트 사용 최대 65535
public: 
	// 서버에 접속 성공한 세션들을 관리하고 있는 배열
	Session* m_pArrSession[MAX_SESSION_CNT]; 

	// 사용 할 수 있는 세션들 모음 (e.g. 재사용 등등)
	std::vector<USHORT> m_usableIdxStack;
	SRWLOCK m_usableIdxStackLock;

};

extern CLanServer *g_Server;
