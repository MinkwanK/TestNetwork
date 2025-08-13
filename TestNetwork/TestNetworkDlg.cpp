
// TestNetworkDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "TestNetwork.h"
#include "TestNetworkDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestNetworkDlg 대화 상자



CTestNetworkDlg::CTestNetworkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTNETWORK_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestNetworkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_IP, m_edIP);
	DDX_Control(pDX, IDC_EDIT_PORT, m_edPort);
	DDX_Control(pDX, IDC_LIST_LOG, m_lstLog);
}

BEGIN_MESSAGE_MAP(CTestNetworkDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CTestNetworkDlg::OnBnClickedButtonConnect)
	ON_MESSAGE(UM_NETWORK_EVENT, &CTestNetworkDlg::OnNetworkEvent)
END_MESSAGE_MAP()


// CTestNetworkDlg 메시지 처리기

BOOL CTestNetworkDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	Init();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CTestNetworkDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CTestNetworkDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CTestNetworkDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTestNetworkDlg::OnBnClickedButtonConnect()
{
	CString sValue; m_edPort.GetWindowText(sValue);
	int iPort = _ttoi(sValue);
	m_edIP.GetWindowText(sValue);

	if (!m_client.GetConnect())
	{
		m_client.SetIPPort(sValue, iPort);
		if (m_client.StartClient())
		{
			GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Disconnect"));
		}
	}
	else
	{
		m_client.StopClient();
		GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Connect"));
	}
}

void CTestNetworkDlg::Init()
{
	m_edIP.SetWindowText(_T("127.0.0.1"));
	m_edPort.SetWindowText(_T("12345"));
	m_client.SetOwner(this);
}

void CTestNetworkDlg::AddLog(CString sLog)
{
	m_lstLog.InsertString(0, sLog);
}

LRESULT CTestNetworkDlg::OnNetworkEvent(WPARAM wParam, LPARAM lParam)
{
	NETWORK_EVENT eEvent = (NETWORK_EVENT)wParam;
	PACKET* pPacket = (PACKET*)lParam;
	CString sValue;

	switch (eEvent)
	{
	case neConnect:
	{
		sValue.Format(_T("접속 성공"));
		AddLog(sValue);
	}break;
	case neDisconnect:
	{
		sValue.Format(_T("연결 끊김"));
		AddLog(sValue);
	}break;
	case neRecv:
	{
		if (pPacket)
		{
			sValue.Format(_T("수신: %s"), pPacket->pszData);
			AddLog(sValue);
		}
	}break;
	}
	

	if (pPacket) delete pPacket;
	return S_OK;
}
