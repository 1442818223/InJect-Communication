
// R3Dlg.h: 头文件
//

#pragma once


// CR3Dlg 对话框
class CR3Dlg : public CDialogEx
{
// 构造
public:
	CR3Dlg(CWnd* pParent = nullptr);	// 标准构造函数


	static void ToggleProtection(int enable);
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_R3_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();

	
	int 初始化 = 0;
};
