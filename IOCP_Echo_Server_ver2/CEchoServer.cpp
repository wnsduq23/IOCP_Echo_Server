#include "pch.h"
#include "Session.h"
#include "CEchoServer.h"
#include "CLanServer.h"

bool CEchoServer::OnConnectionRequest(const WCHAR* ip, USHORT port)
{
	// ���� ��� IP���� Ȯ�� �� false�� accept X
	return true;
}

void CEchoServer::OnAccept(const __int64 sessionID)
{
	// Ŭ���̾�Ʈ ���� ����
	// wprintf(L"[Content] Client Join : %lld\n", sessionID);
}

void CEchoServer::OnClientLeave(const __int64 sessionID)
{
	// Ŭ���̾�Ʈ ���� ó��
	// wprintf(L"[Content] Client Leave : %lld\n", sessionID);
}

void CEchoServer::OnMessage(const __int64 sessionID, SerializePacket* message)
{
	// Content ��� Ȯ�� -> ó��
	// ������ �׳� ���

	__int64 num;
	*message >> num;
	 //printf("[Content] Client ID : %lld, Recv Msg : %lld\n", sessionID, num);

	SerializePacket packet;
	packet << (SHORT)sizeof(num) << num;
	g_Server->SendPacket(sessionID, &packet);
}

void CEchoServer::OnError(int errorcode, WCHAR *errMsg)
{

}