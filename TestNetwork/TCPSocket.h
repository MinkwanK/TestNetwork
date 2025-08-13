#pragma once
#include <atomic>

#define UM_NETWORK_EVENT	(WM_USER + 1000)
/*
 
WSANOTINITIALISED (10093)	WSAStartup() 호출 안 됨
WSAENETDOWN (10050)			네트워크 서브시스템 다운
WSAEINVAL (10022)			잘못된 파라미터 (fd_set에 잘못된 소켓 포함 등)
WSAEFAULT (10014)			잘못된 메모리 참조
WSAEINTR (10004)			차단 호출이 취소됨
WSAENOTSOCK (10038)			SOCKET 핸들이 아님

 */
typedef enum NETWORK_EVENT
{
	neConnect,
	neDisconnect,
	neRecv,
	neSend,
};

typedef struct PACKET
{
	SOCKET sock;
	char* pszData;
	UINT uiSize;

}PACKET, * PACKET_PTR;

constexpr int MAX_BUF = 1024;

class TCPSocket
{
public:
	TCPSocket();
	virtual ~TCPSocket();

public:
	void SetIPPort(const CString& sIP, const UINT uiPort) { m_sIP = sIP; m_uiPort = uiPort; }
	int GetSocket() const { return m_sock; }
	void Close();
	void SetOwner(CWnd* pOwner) { m_pOwner = pOwner; }
	CWnd* GetOwner() const { return m_pOwner; }

protected:
	bool InitWinSocket();
	void CloseWinSocket();
	void CloseSocket();
	virtual bool MakeNonBlockingSocket() = NULL;
	virtual void SendProc(SOCKET sock) = NULL;
	virtual void RecvProc(SOCKET sock) = NULL;
	virtual int Send() = NULL;
	virtual int Read(SOCKET sock) = NULL;

protected:
	//Resource
	CWnd* m_pOwner = nullptr;

	//Network Resource
	SOCKET m_sock;
	CString m_sIP;
	UINT m_uiPort = -1;

	//Thread
	HANDLE m_hRecvThread = nullptr;
	HANDLE m_hSendThread = nullptr;

	//Exit
	std::atomic<bool> m_bStop = false;
	bool GetStop() const { return m_bStop; }
	void SetStop(const bool bStop) { m_bStop = bStop; }

	//Lock
	CRITICAL_SECTION m_cs;
};

