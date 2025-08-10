#pragma once
#include "TCPSocket.h"
class Client : public TCPSocket
{
public:
	Client();
	~Client();

public:
	bool StartClient();

protected:
	//Socket
	bool MakeNonBlockingSocket() override;

	//CONNECT
	static bool ConnectThread(Client* pClient);
	bool ConnectProc();
	bool Connect();

	//SEND
	static void SendThread(Client* pClient, SOCKET sock);
	void SendProc(SOCKET sock) override;
	int Send() override;

	//RECV
	static void RecvThread(Client* pClient, SOCKET sock);
	void RecvProc(SOCKET sock) override;
	int Read(SOCKET sock) override;

private:
	sockaddr_in m_serverAddr{}; //접속 대상 서버의 sockaddr_in
};

