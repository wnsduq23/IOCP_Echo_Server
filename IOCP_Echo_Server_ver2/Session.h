#pragma once

#include "pch.h"

// ��Ʈ��ũ I/O ����
enum class NET_TYPE { RECV, SEND };

// I/O overlapped ����ü Ȯ��
struct NetworkOverlapped
{
    OVERLAPPED _ovl;
    NET_TYPE _type;
    // �ʿ��� �߰� �ʵ尡 ������ �߰�
};

class Session
{
public:
    // ������: session ID, ����, Ŭ���̾�Ʈ �ּ� ����
    Session(__int64 ID, SOCKET sock, SOCKADDR_IN addr)
        : m_SessionID(ID), m_ClientSock(sock), m_ClientAddr(addr), m_RecvBuf(), m_SendBuf(),
        m_IOCount(0), m_SendFlag(0), m_IsValid(TRUE)
    {
		InitializeCriticalSection(&_cs);
        ZeroMemory(&_recvOvl, sizeof(_recvOvl));
        ZeroMemory(&_sendOvl, sizeof(_sendOvl));

        // WSABUF �迭 �ʱ�ȭ (�� ���� ���)
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

    // Public ��� ���� (���� ������Ʈ������ ĸ��ȭ�� ����� getter/setter ���)
    __int64 m_SessionID;
    SOCKET m_ClientSock;
    SOCKADDR_IN m_ClientAddr;

    // ���� �� �۽� ����
    RingBuffer m_RecvBuf;
    RingBuffer m_SendBuf;

    // Overlapped I/O ����ü (����/�۽�)
    NetworkOverlapped _recvOvl;
    NetworkOverlapped _sendOvl;

    // WSABUF �迭 (I/O ��û �� ���)
    WSABUF _wsaRecvbuf[2];
    WSABUF _wsaSendbuf[2];

    // I/O ���� ī���� �� �÷���
    // ����ȭ ��ü�� ����� ��� ���� ī���� ����� �ؾ��Ѵ�. 
    // �̸� IOCount�� ������ Ȯ���ؾ��� �ʿ䰡 ����.
    volatile LONG m_IOCount = 0;
    volatile LONG m_SendFlag = 0;
    volatile LONG m_IsValid = 0; // ������ �ڵ� �Լ� �� ����

    // �߰������� �ʿ��� �Լ����� ���� ����
    CRITICAL_SECTION _cs;

    // �� ���� �޸�Ǯ �߰�
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
