#pragma once

constexpr const CHAR *SERVER_IP = "0.0.0.0";
constexpr USHORT SERVER_PORT = 6000;
#define MAX_BUFFER_SIZE 1024

// ������ ���� ����
#define MAX_SESSION_CNT 65535

// ��������: ��� ���� 2 Byte, ������ ���� 8 Byte (����)
#define HEADER_LEN sizeof(stHeader)
//#define dfDATA_LEN 8

struct stHeader
{
	short _shLen;
};
