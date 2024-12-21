
// R3Dlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "R3.h"
#include "R3Dlg.h"
#include "afxdialogex.h"


#include "LoadDriver.h"
#include "api.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// 定义与驱动程序通信的符号链接
#define DEVICE_LINK_NAME "\\\\.\\tearWrite_driver"
// 定义IOCTL控制码
#define IOCTL_IO_SetPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2021, METHOD_BUFFERED, FILE_ANY_ACCESS)
static HANDLE hDevice = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CR3Dlg 对话框



CR3Dlg::CR3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_R3_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CR3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CR3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CR3Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CR3Dlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CR3Dlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CR3Dlg 消息处理程序

BOOL CR3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CR3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CR3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CR3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
HANDLE  通信初始化() {

	hDevice = CreateFileA(DEVICE_LINK_NAME, FILE_ALL_ACCESS,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE) {
		AfxMessageBox(_T("无法打开设备"));
		return NULL;
	}
	return hDevice;
}
void 关闭通信()
{
	if (hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDevice);
		hDevice = INVALID_HANDLE_VALUE;
	}
}
void CR3Dlg::ToggleProtection(int enable)
{

	// 加入锁定机制来确保线程安全
	static std::mutex deviceMutex;
	std::lock_guard<std::mutex> lock(deviceMutex);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(_T("Device not opened"));
		return;
	}

	int switchValue = enable;
	DWORD bytesReturned;
	BOOL result = DeviceIoControl(hDevice, IOCTL_IO_SetPROTECT,
		&switchValue, sizeof(switchValue),
		NULL, 0,
		&bytesReturned, NULL);

	if (!result)
	{
		AfxMessageBox(_T("Failed to send IOCTL request"));
	}
}




int 驱动是否加载 = 0;
UINT LoadDriverThread(LPVOID pParam)
{
	if (驱动是否加载 == 0)
	{
		驱动是否加载 = 1;  // 设置驱动已加载的标志

		if (SH_DriverLoad())  // 加载驱动函数
		{

			通信初始化();
			Beep(2000, 1000);  // 加载成功后发出蜂鸣音
		}
		else
		{
			AfxMessageBox(_T("初始化失败!"));
		}
	}
	else
	{
		AfxMessageBox(_T("驱动已经被加载!"));
	}

	return 0;  // 线程完成
}
void CR3Dlg::OnBnClickedButton1()
{
	if (驱动是否加载 == 0)
	{
	
		// 创建线程，执行加载驱动的操作
		AfxBeginThread(LoadDriverThread, NULL);

		// 弹窗提示用户上游戏
		AfxMessageBox(_T("驱动正在加载，请启动游戏！"));
	
	
	}
	else
	{
		// 如果驱动已经加载，弹出提示
		AfxMessageBox(_T("驱动已经被加载！"));
	}

}


void CR3Dlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	ToggleProtection(1);
	Beep(1000, 500);



}


void CR3Dlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码

	关闭通信();
	if (SH_UnDriverLoad()) {


		Beep(1000, 500);
		//	AfxMessageBox(_T("卸载成功!"));
	}
}
