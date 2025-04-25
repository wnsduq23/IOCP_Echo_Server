#pragma once

constexpr const CHAR *SERVER_IP = "0.0.0.0";
constexpr USHORT SERVER_PORT = 6000;
#define MAX_BUFFER_SIZE 1024

// 세션의 수가 유저
#define MAX_SESSION_CNT 65535

// 프로토콜: 헤더 길이 2 Byte, 데이터 길이 8 Byte (에코)
#define HEADER_LEN sizeof(stHeader)
//#define dfDATA_LEN 8

struct stHeader
{
	short _shLen;
};
