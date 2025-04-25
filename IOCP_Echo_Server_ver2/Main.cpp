#include "pch.h"
#include "Session.h"
#include "CLanServer.h"
#include "CEchoServer.h"
#include "Main.h"

CLanServer* g_Server;

bool g_bShutdown = false;
int main()
{
	// ���� �ν��Ͻ� ����
	g_Server = new CEchoServer;

	g_Server->Start(SERVER_IP, SERVER_PORT, 8, 8, 65535);
	// ���� ���� �� (g_bShutdown�� true�� �� ������ ����)
	while (!g_bShutdown)
	{
		Sleep(1000);
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_bShutdown = true;
		}
	}
	printf("���� ����\n");

	return 0;
}