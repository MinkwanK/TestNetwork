#include "pch.h"
#include "TCPSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

TCPSocket::TCPSocket()
{
	InitWinSocket();
    InitializeCriticalSection(&m_cs);
}

TCPSocket::~TCPSocket()
{
	CloseSocket();
	CloseWinSocket();
	DeleteCriticalSection(&m_cs); // «ÿ¡¶
}

bool TCPSocket::InitWinSocket()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	return result == 0;
}

void TCPSocket::CloseWinSocket()
{
	WSACleanup();
}

void TCPSocket::CloseSocket()
{
	EnterCriticalSection(&m_cs);
	if (m_sock)
	{
		closesocket(m_sock);
		m_sock = NULL;
	}
	LeaveCriticalSection(&m_cs);
}

void TCPSocket::Close()
{
	SetStop(true);
}
