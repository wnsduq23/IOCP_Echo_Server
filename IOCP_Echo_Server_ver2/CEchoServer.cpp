#include "pch.h"
#include "Session.h"
#include "CEchoServer.h"
#include "CLanServer.h"

bool CEchoServer::OnConnectionRequest(const WCHAR* ip, USHORT port)
{
	// 접속 허용 IP인지 확인 후 false면 accept X
	return true;
}

void CEchoServer::OnAccept(const __int64 sessionID)
{
	// 클라이언트 정보 생성
	// wprintf(L"[Content] Client Join : %lld\n", sessionID);
}

void CEchoServer::OnClientLeave(const __int64 sessionID)
{
	// 클라이언트 종료 처리
	// wprintf(L"[Content] Client Leave : %lld\n", sessionID);
}

void CEchoServer::OnMessage(const __int64 sessionID, SerializePacket* message)
{
	// Content 헤더 확인 -> 처리
	// 지금은 그냥 출력

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