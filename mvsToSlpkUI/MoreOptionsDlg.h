#pragma once
#include "afxdialogex.h"


// MoreOptionsDlg dialog

class MoreOptionsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MoreOptionsDlg)

public:
	MoreOptionsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~MoreOptionsDlg();

	enum { IDD = IDD_MORE_OPT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_process;
	int m_slpk;
	virtual void OnOK();
};
