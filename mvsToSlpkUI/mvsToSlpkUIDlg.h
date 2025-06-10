#pragma once
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "ProjectDivision.h"

const UINT APP_FOLDER_SELECTED = WM_APP + 4;
const UINT THREAD_ENDED = WM_APP + 1;
const UINT APP_PROGESS_PIPE = WM_APP + 5;

#include "MFCEditBrowseCtrlEx.h"

class My_CMFCEditBrowseCtrl2 : public MFCEditBrowseCtrlEx
{
public:
	virtual void OnBrowse()
	{
		MFCEditBrowseCtrlEx::OnBrowse();
		GetParent()->PostMessage(APP_FOLDER_SELECTED);
	}
};


// CmvsToSlpkUIDlg dialog
class CmvsToSlpkUIDlg : public CDialogEx
{
// Construction
public:
	CmvsToSlpkUIDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MVSTOSLPKUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnFolderSelected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnThreadComplete(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnProgressPipe(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	My_CMFCEditBrowseCtrl2 m_editBrowse_folder;
	CProgressCtrl m_progress;
	CString m_info;
	afx_msg void OnBnClickedButtonProcess();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();
	MFCEditBrowseCtrlEx m_outputFolder;

	void processIt(const std::wstring fullExePath, const std::wstring cmdLineStr);
	//std::thread m_thread;
	CButton m_cancelButton;
	bool checkToTerminate();

	void progressServer();
	std::thread m_threadPS;
	void endProgress();

	CStatic m_infoStatic;
	CComboBox m_maxImageCombo;
	CString m_maxImage;

	HWND _consoleHwnd = nullptr;
	DWORD _consolePID = 0;
	HANDLE _consoleHandle = nullptr;
	MFCEditBrowseCtrlEx m_smtxmlEdit;
	MFCEditBrowseCtrlEx m_prjFileEdit;
	CString m_smtxml;
	CString m_prjFile;

	void processMultipleProject(ProjectDivision::ProjectDiv param, int maxImageSize, CString workingDir, CString outputDir, std::wstring fullExePath);
	std::thread m_bkThread;
	std::atomic<bool> stopThread = false;

	void processItBase(const std::wstring fullExePath, const std::wstring cmdLineStr);
	int m_radio;
	BOOL m_openmvsSplit;
	CString m_splitMaxImages;

	void saveToReg();
};
