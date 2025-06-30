// MoreOptionsDlg.cpp : implementation file
//

#include "pch.h"
#include "mvsToSlpkUI.h"
#include "afxdialogex.h"
#include "MoreOptionsDlg.h"


// MoreOptionsDlg dialog

IMPLEMENT_DYNAMIC(MoreOptionsDlg, CDialogEx)

MoreOptionsDlg::MoreOptionsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MORE_OPT, pParent)
	, m_process(0)
	, m_slpk(0)
{

}

MoreOptionsDlg::~MoreOptionsDlg()
{
}

void MoreOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_COMBO_PROCESS, m_process);
	DDX_CBIndex(pDX, IDC_COMBO_SLJK_DIV, m_slpk);
}


BEGIN_MESSAGE_MAP(MoreOptionsDlg, CDialogEx)
END_MESSAGE_MAP()


// MoreOptionsDlg message handlers

void MoreOptionsDlg::OnOK()
{
	UpdateData(TRUE);

	CDialogEx::OnOK();
}
