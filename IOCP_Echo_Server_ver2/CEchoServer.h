#pragma once
#include "CLanServer.h"
class CEchoServer : public CLanServer
{
public:
	bool OnConnectionRequest(const WCHAR* ip, USHORT port);
	void OnAccept(const __int64 sessionID); // Accept �� ���� ó�� �Ϸ� �� ȣ��
	void OnClientLeave(const __int64 sessionID); // Release �� ȣ��
	void OnMessage(const __int64 sessionID, SerializePacket* message); // ��Ŷ ���� �Ϸ� ��
	void OnError(int errorcode, WCHAR* errMsg);
};
