#pragma once

#include "pch.h"

// 네트워크 I/O 종류
enum class NET_TYPE { RECV, SEND };

// I/O overlapped 구조체 확장
struct NetworkOverlapped
{
    OVERLAPPED _ovl;
    NET_TYPE _type;
    // 필요한 추가 필드가 있으면 추가
};

class Session
{
public:
    // 생성자: session ID, 소켓, 클라이언트 주소 전달
    Session(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
        : m_SessionID(ID), m_ClientSock(sock), m_ClientAddr(addr), m_RecvBuf(), m_SendBuf(),
        m_IOCount(0), m_SendFlag(0), m_IsValid(TRUE)
    {
		InitializeCriticalSection(&_cs);
        ZeroMemory(&_recvOvl, sizeof(_recvOvl));
        ZeroMemory(&_sendOvl, sizeof(_sendOvl));

        // WSABUF 배열 초기화 (두 버퍼 사용)
        _wsaRecvbuf[0].buf = nullptr;
        _wsaRecvbuf[0].len = 0;
        _wsaRecvbuf[1].buf = nullptr;
        _wsaRecvbuf[1].len = 0;

        _wsaSendbuf[0].buf = nullptr;
        _wsaSendbuf[0].len = 0;
        _wsaSendbuf[1].buf = nullptr;
        _wsaSendbuf[1].len = 0;
    }
    ~Session()
    {
        DeleteCriticalSection(&_cs);
    }

	// I/O Completion handler functions
	void HandleRecvCP(int recvBytes);
	void HandleSendCP(int sendBytes);

	// Functions to post I/O requests
	void RecvPost();
	void SendPost();

	// Message processing functions
	void RecvDataToMsg();
	void SendPacketQueue(SerializePacket* packet);

	void Initialize(__int64 sessionID, SOCKET socket, SOCKADDR_IN addr)
	{
		m_ClientSock = socket;
		m_SessionID = sessionID;
        m_ClientAddr = addr;
		m_SendFlag = FALSE;
		//InterlockedExchange(&m_IsValid, TRUE);
		m_IOCount = 0;
		//m_SendCount = 0;
		m_SendBuf.ClearBuffer();
		m_RecvBuf.ClearBuffer();
        InterlockedExchange(&m_IsValid, TRUE);
	}

	void Clear()
	{
		//InterlockedExchange(&m_IsValid, FALSE);
		m_IOCount = 0;
		m_SendFlag = 0;
		m_ClientSock = INVALID_SOCKET;
		m_SessionID = -1;

		m_RecvBuf.ClearBuffer();
		m_SendBuf.ClearBuffer();

        InterlockedExchange(&m_IsValid, FALSE);
	}

    // Public 멤버 변수 (실제 프로젝트에서는 캡슐화를 고려해 getter/setter 사용)
    __int64 m_SessionID;
    SOCKET m_ClientSock;
    SOCKADDR_IN m_ClientAddr;

    // 수신 및 송신 버퍼
    RingBuffer m_RecvBuf;
    RingBuffer m_SendBuf;

    // Overlapped I/O 구조체 (수신/송신)
    NetworkOverlapped _recvOvl;
    NetworkOverlapped _sendOvl;

    // WSABUF 배열 (I/O 요청 시 사용)
    WSABUF _wsaRecvbuf[2];
    WSABUF _wsaSendbuf[2];

    // I/O 관련 카운터 및 플래그
    // 동기화 객체를 지우는 대신 참조 카운팅 방식을 해야한다. 
    // 이를 IOCount의 역할을 확장해야할 필요가 있음.
    volatile LONG m_IOCount = 0;
    volatile LONG m_SendFlag = 0;
    volatile LONG m_IsValid = 0; // 세션의 핸들 함수 못 들어가게

    // 추가적으로 필요한 함수들은 여기 선언
    CRITICAL_SECTION _cs;

    // 락 프리 메모리풀 추가
    /*public:
	static Session *Alloc()
	{
		return m_SessionLFMemoryPool.Alloc();
	}

	static void Free(Session *ptr)
	{
		m_SessionLFMemoryPool.Free(ptr);
	}

	inline static LFMemoryPool<Session> m_SessionLFMemoryPool = LFMemoryPool<Session>(100, false);*/
};
