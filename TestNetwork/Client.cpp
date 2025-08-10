#include "pch.h"
#include "Client.h"
#include <WS2tcpip.h>
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Client::Client()
{
}

Client::~Client()
{

}

bool Client::MakeNonBlockingSocket()
{
    CString sValue;

    // 1. ���� �ʱ�ȭ
    CloseSocket();

    // 2. ���� ����
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) 
    {
        OutputDebugString(_T("Socket creation failed.\n"));
        return false;
    }

    // 3. ����ŷ ��� ����
    u_long mode = 1;
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) 
    {
        OutputDebugString(_T("ioctlsocket failed\n"));
        return false;
    }

    // 4. ���� �ּ� ����;
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_uiPort);
    if (InetPton(AF_INET, m_sIP, &m_serverAddr.sin_addr) != 1)    //InetPton�� ���ڿ� IP�� ���� IP�� �����ϰ� ��ȯ
    {
        OutputDebugString(_T("Invalid IP address format\n"));
        return false;
    }
    return true;
}

bool Client::StartClient()
{
    if (!MakeNonBlockingSocket())
    {
        OutputDebugString(_T("StartClient Failed\n"));
    }

    std::thread ConnectThread(&Client::ConnectThread, this);
    ConnectThread.detach();
    return TRUE;
}

bool Client::ConnectThread(Client* pClient)
{
    if (pClient)
    {
        return pClient->ConnectProc();
    }
    return false;
}

bool Client::ConnectProc()
{
    OutputDebugString(_T("Connect ����\n"));

    if (m_sock == INVALID_SOCKET)
        return false;

    if (!Connect())
    {
        return false;
    }

    CString sValue;
    while (!m_bStop)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 1; // �ִ� 1�� ���
        timeout.tv_usec = 0;

        const int iResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //�����Ǹ� Ŭ���̾�Ʈ ���� ����
        {
            sValue.Format(_T("[Ŭ���̾�Ʈ] ����: %d ����\n"), m_sock);
            OutputDebugString(sValue);
            break;
        }
        else 
        {
            if (iResult == 0)
            {
                sValue.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient CONNECT Ÿ�Ӿƿ�\n"));
                OutputDebugString(sValue);
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();
                CString sMsg;
                sMsg.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient - select() ���� - WSAGetLastError(): %d\n"), iError);
                OutputDebugString(sMsg);
                return false;
            }
        }
    }

    std::thread sendThread(&Client::SendThread, this, m_sock);
    sendThread.detach();

    std::thread recvThread(&Client::RecvThread, this, m_sock);
    recvThread.detach();
    return true;
}

bool Client::Connect()
{
	//�񵿱� ����
    const int iResult = connect(m_sock, reinterpret_cast<SOCKADDR*>(&m_serverAddr), sizeof(m_serverAddr));
    if (iResult == SOCKET_ERROR)
    {
        int iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS )  
        {
            CString sValue;
            sValue.Format(_T("Connect Error: %d\n"),iError);
            OutputDebugString(sValue);
	        return false;
        }
    }
    return true;
}

void Client::SendThread(Client* pClient, SOCKET sock)
{
    if (pClient)
    {
        pClient->SendProc(sock);
    }
}

void Client::SendProc(SOCKET sock)
{
    CString sValue;
    while (!m_bStop)
    {
        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }
        // select �غ�
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5�� ���

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);  
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] SEND ����: select (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(sock, &writeSet))
        {
            int iResult = Send();
            if (iResult == 0)
            {
                break;
            }
        }
    }
}

int Client::Send()
{
    //EnterCriticalSection(&m_cs);
    //CString sValue;
    //const int iSendCount = m_aSend.GetCount();
    //int iSend = -1;
    //if (iSendCount > 0)
    //{
    //    PACKET packet = m_aSend.GetAt(0);
    //    SOCKET targetSock = packet.sock;  // ��Ŷ ���� �ִ� ��Ȯ�� ���� ���

    //    if (targetSock != INVALID_SOCKET)
    //    {
    //        iSend = send(targetSock, packet.pszData, packet.uiSize, 0);
    //        if (iSend > 0)
    //             sValue.Format(_T("[Client][SEND] %d ���� %d ����Ʈ\n"), targetSock, iSend);
    //        else if (iSend == 0)
    //            sValue.Format(_T("[Client][SEND] %d ���� ���� ���� (������ ���� ����)\n"), targetSock);
    //        else
    //        {
    //            sValue.Format(_T("[Client][SEND] %d ���� �����ڵ� %d\n"), targetSock, WSAGetLastError());
    //            return false;
    //        }
    //        
    //        //UseCallback(NETWORK_EVENT::SEND, packet, targetSock, sValue);
    //    }
    //}
    //LeaveCriticalSection(&m_cs);
    //return iSend;
    return 0;
}

void Client::RecvThread(Client* pClient, SOCKET sock)
{
    if (pClient)
    {
        pClient->RecvProc(sock);
    }
}

void Client::RecvProc(SOCKET sock)
{
    CString sValue;
    while (!m_bStop)
    {
        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        // select �غ�
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5�� ���

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Recv ����: select (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(sock, &readSet))
        {
           int iResult = Read(sock);
            if (iResult < 0)
            {
                break;
            }
            else if (iResult == 0) //���� ����
            {
                sValue.Format(_T("[Client][RECV] %d ���� ���� ���� (������ ���� ����)\n"), sock);
                OutputDebugString(sValue);
                m_bStop = TRUE;
                break;
            }
        }
    }
    SetEvent(m_hRecvThread);
}

int Client::Read(SOCKET sock)
{
    CString sValue;
    char buf[MAX_BUF];
    int iRecv = recv(sock, buf, MAX_BUF, 0);

    if (iRecv > 0)
    {
        sValue.Format(_T("[Client][RECV] %d ���� %d ����Ʈ\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Client][RECV] %d ���� ���� ���� (������ ���� ����)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Client][RECV] %d ���� �����ڵ� %d\n"), sock, WSAGetLastError());
        return false;
    }

    PACKET packet = {};

    if (iRecv > 0)
    {
        packet.pszData = buf;
        packet.uiSize = iRecv;

        CString sRead(buf);
        sRead.AppendFormat(_T("\t���� \n"));
        OutputDebugString(sRead);
    }

    OutputDebugString(sValue);
    return iRecv;
}
