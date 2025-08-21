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

    // 1. 소켓 초기화
    CloseSocket();

    // 2. 소켓 생성
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) 
    {
        OutputDebugString(_T("Socket creation failed.\n"));
        return false;
    }

    // 3. 논블로킹 모드 설정
    u_long mode = 1;
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) 
    {
        OutputDebugString(_T("ioctlsocket failed\n"));
        return false;
    }

    // 4. 서버 주소 설정;
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_uiPort);
    if (InetPton(AF_INET, m_sIP, &m_serverAddr.sin_addr) != 1)    //InetPton은 문자열 IP를 이진 IP로 안전하게 변환
    {
        OutputDebugString(_T("Invalid IP address format\n"));
        return false;
    }
    return true;
}

bool Client::StartClient()
{
    m_bStop = false;
    if (!MakeNonBlockingSocket())
    {
        OutputDebugString(_T("StartClient Failed\n"));
    }

    std::thread ConnectThread(&Client::ConnectThread, this);
    ConnectThread.detach();
    SetRunning(true);
    return TRUE;
}

void Client::StopClient()
{
    m_bStop = true;
    if (m_sock)
    {
        closesocket(m_sock);
        m_sock = -1;
    }

    PostMessage(m_pOwner->GetSafeHwnd(), UM_NETWORK_EVENT, (WPARAM)neDisconnect, (LPARAM)nullptr);
    SetConnect(false);
    SetRunning(false);
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
    OutputDebugString(_T("Connect 시작\n"));

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
        timeout.tv_sec = 1; // 최대 1초 대기
        timeout.tv_usec = 0;

        const int iResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //감지되면 클라이언트 연결 성공
        {
            sValue.Format(_T("[클라이언트] 성공: %d 접속\n"), m_sock);
            OutputDebugString(sValue);

            if (m_pOwner && m_pOwner->GetSafeHwnd())
            {
                PostMessage(m_pOwner->GetSafeHwnd(), UM_NETWORK_EVENT, (WPARAM)neConnect, (LPARAM)nullptr);
            }
            SetConnect(true);
            break;
        }
        else 
        {
            if (iResult == 0)
            {
                sValue.Format(_T("[클라이언트] 에러: StartClient CONNECT 타임아웃\n"));
                OutputDebugString(sValue);
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();
                CString sMsg;
                sMsg.Format(_T("[클라이언트] 에러: StartClient - select() 실패 - WSAGetLastError(): %d\n"), iError);
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
	//비동기 연결
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
            sValue.Format(_T("[Client] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }
        // select 준비
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5초 대기

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);  
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] SEND 오류: select (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

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
    EnterCriticalSection(&m_cs);
    CString sValue;
    const int iSendCount = m_aSend.GetCount();
    int iSend = -1;
    if (iSendCount > 0)
    {
        PACKET packet = m_aSend.GetAt(0);
        SOCKET targetSock = packet.sock;  // 패킷 내에 있는 정확한 소켓 사용

        if (targetSock != INVALID_SOCKET)
        {
            iSend = send(targetSock, packet.pszData, packet.uiSize, 0);
            if (iSend > 0)
            {
                sValue.Format(_T("[Client][SEND] %d 소켓 %d 바이트\n"), targetSock, iSend);

                PACKET* pPostMessagePacket = new PACKET;
                pPostMessagePacket->pszData = packet.pszData;
                pPostMessagePacket->uiSize = iSend;

                CString sSend(packet.pszData);
                sSend.AppendFormat(_T("\t송신 \n"));
                OutputDebugString(sSend);

                if (m_pOwner && m_pOwner->GetSafeHwnd())
                {
                    PostMessage(m_pOwner->GetSafeHwnd(), UM_NETWORK_EVENT, (WPARAM)neSend, (LPARAM)pPostMessagePacket);
                }
                else
                {
                    if (pPostMessagePacket)
                    {
                        if(pPostMessagePacket->pszData) delete[] pPostMessagePacket->pszData;
                        delete pPostMessagePacket;
                    }
                }
            }
            else if (iSend == 0)
            {
                sValue.Format(_T("[Client][SEND] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), targetSock);
                delete[] packet.pszData;
            }
            else
            {
                sValue.Format(_T("[Client][SEND] %d 소켓 실패코드 %d\n"), targetSock, WSAGetLastError());
                delete[] packet.pszData;
                return false;
            }
        }
        m_aSend.RemoveAt(0);
    }
    OutputDebugString(sValue);
    LeaveCriticalSection(&m_cs);
    return iSend;
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
            sValue.Format(_T("[Client] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        // select 준비
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5초 대기

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Recv 오류: select (%d)"), WSAGetLastError());
            OutputDebugString(sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(sock, &readSet))
        {
           int iResult = Read(sock);
            if (iResult < 0)
            {
                break;
            }
            else if (iResult == 0) //접속 종료
            {
                sValue.Format(_T("[Client][RECV] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), sock);
                OutputDebugString(sValue);
                m_bStop = TRUE;
                break;
            }
        }
    }
}

int Client::Read(SOCKET sock)
{
    CString sValue;
    char* pBuf = new char[MAX_BUF];
    int iRecv = recv(sock, pBuf, MAX_BUF, 0);

    if (iRecv > 0)
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 %d 바이트\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 실패코드 %d\n"), sock, WSAGetLastError());
        return false;
    }

    if (iRecv > 0)
    {
        PACKET* pPacket = new PACKET;
        pPacket->pszData = pBuf;
        pPacket->uiSize = iRecv;

        CString sRead(pBuf);
        sRead.AppendFormat(_T("\t수신 \n"));
        OutputDebugString(sRead);

        if (m_pOwner && m_pOwner->GetSafeHwnd())
        {
            PostMessage(m_pOwner->GetSafeHwnd(), UM_NETWORK_EVENT, (WPARAM)neRecv, (LPARAM)pPacket);
        }
        else
        {
            if (pPacket)
            {
                if (pPacket->pszData) delete[] pPacket->pszData;
                delete pPacket;
            }
        }
    }
    OutputDebugString(sValue);
    return iRecv;
}
