
// mvsToSlpkUIDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "mvsToSlpkUI.h"
#include "mvsToSlpkUIDlg.h"
#include "afxdialogex.h"
#include "StdUtil/StdUtility.h"
#include "StdUtil/WinUtility.h"
#include "ProjectDivision.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

const wchar_t* REG_KEY_NAME = L"CmvsToSlpkUIDlg";

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// CmvsToSlpkUIDlg dialog



CmvsToSlpkUIDlg::CmvsToSlpkUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MVSTOSLPKUI_DIALOG, pParent)
	, m_info(_T(""))
	, m_maxImage(_T("1000"))
	, m_smtxml(_T(""))
	, m_prjFile(_T(""))
	, m_radio(1)
	, m_openmvsSplit(FALSE)
	, m_splitMaxImages(_T("200"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CmvsToSlpkUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_FOLDER, m_editBrowse_folder);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
	DDX_Text(pDX, IDC_STATIC_INFO, m_info);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_results, m_outputFolder);
	DDX_Control(pDX, IDCANCEL, m_cancelButton);
	DDX_Control(pDX, IDC_STATIC_INFO, m_infoStatic);
	DDX_Control(pDX, IDC_COMBO_MAX_IMAGE, m_maxImageCombo);
	DDX_CBString(pDX, IDC_COMBO_MAX_IMAGE, m_maxImage);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_SMTXML, m_smtxmlEdit);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_PRJ, m_prjFileEdit);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_SMTXML, m_smtxml);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_PRJ, m_prjFile);
	DDX_Radio(pDX, IDC_RADIO_SINGLE_PROJ, m_radio);
	DDX_Check(pDX, IDC_CHECK_OPENMVS_SPLIT, m_openmvsSplit);
	DDX_CBString(pDX, IDC_COMBO_SPLIT_AMT, m_splitMaxImages);
}

BEGIN_MESSAGE_MAP(CmvsToSlpkUIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PROCESS, &CmvsToSlpkUIDlg::OnBnClickedButtonProcess)
	ON_MESSAGE(APP_FOLDER_SELECTED, &CmvsToSlpkUIDlg::OnFolderSelected)
	ON_MESSAGE(THREAD_ENDED, &CmvsToSlpkUIDlg::OnThreadComplete)
	ON_MESSAGE(APP_PROGESS_PIPE, &CmvsToSlpkUIDlg::OnProgressPipe)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CmvsToSlpkUIDlg message handlers

BOOL CmvsToSlpkUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	WinUtility::setRegistryLocation(L"MsvToSlpkUI");


	m_editBrowse_folder.EnableFolderBrowseButton();
	m_outputFolder.EnableFolderBrowseButton();

	m_smtxmlEdit.EnableFileBrowseButton(L"smtxml", L"smtxml|*.smtxml||", OFN_READONLY | OFN_OVERWRITEPROMPT);
	m_prjFileEdit.EnableFileBrowseButton(L"prj", L"prj|*.prj||", OFN_READONLY | OFN_OVERWRITEPROMPT);


	m_progress.SetRange(0, 100);

	WinUtility::loadSetting(REG_KEY_NAME, L"m_radio", m_radio);
	WinUtility::loadSetting(REG_KEY_NAME, L"m_openmvsSplit", m_openmvsSplit);

	std::wstring tmp;
	if (WinUtility::loadSetting(REG_KEY_NAME, L"m_maxImage", tmp)) {
		m_maxImage = tmp.c_str();
	}
	
	if (WinUtility::loadSetting(REG_KEY_NAME, L"m_splitMaxImages", tmp)) {
		m_splitMaxImages = tmp.c_str();
	}
	

	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CmvsToSlpkUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CmvsToSlpkUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CmvsToSlpkUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CmvsToSlpkUIDlg::OnFolderSelected(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);

	CString folderTxt;
	m_editBrowse_folder.GetWindowText(folderTxt);


	if (!StdUtility::fileExists(std::wstring(folderTxt))) {
		return 1;
	}

	// find summit project
	// find PRJ file
	std::vector<std::wstring> foundFilenames;
	std::wstring folder = folderTxt;
	StdUtility::findFiles(foundFilenames, folder, L".*\\.smtxml", true);

	if (foundFilenames.size() > 0) {
		m_smtxml = foundFilenames[0].c_str();
	}

	// find the Prj
	std::vector<std::wstring> foundPrjFiles;
	StdUtility::findFiles(foundPrjFiles, folder, L".*\\.prj", true);
	if (foundPrjFiles.size() > 0) {
		m_prjFile = foundPrjFiles[0].c_str();
	}

	if (foundFilenames.size() == 0) {
		CString msg;
		msg = L"Summit project file not found.\n\n";
		msg += folderTxt;
		AfxMessageBox(msg);
		return 1;
	}
	if (foundPrjFiles.size() == 0) {
		CString msg;
		msg = L"PRJ project file not found.\n\n";
		msg += folderTxt;
		AfxMessageBox(msg);
		return 1;
	}

	// Lets make this a bit better
	// Go up one directory from where the summit file was found
	std::wstring parentDir = StdUtility::getParentPath(std::wstring(m_smtxml));
	std::wstring upOne = StdUtility::getParentPath(parentDir);


	std::wstring meshFolder = upOne;
	StdUtility::appendSlash(meshFolder);
	meshFolder += L"Mesh";

	// 	std::wstring meshFolder = folderTxt;
	// 	StdUtility::appendSlash(meshFolder);
	// 	meshFolder += L"Mesh";

	m_outputFolder.SetWindowTextW(CString(meshFolder.c_str()));

	UpdateData(FALSE);

	return 1;
}

void CmvsToSlpkUIDlg::OnBnClickedButtonProcess()
{
	UpdateData(TRUE);

	// check for project files
	if (m_smtxml.IsEmpty() || !StdUtility::fileExists(std::wstring(m_smtxml))) {
		AfxMessageBox(L"SMTXML file not found.");
		return;
	}

	if (m_prjFile.IsEmpty() || !StdUtility::fileExists(std::wstring(m_prjFile))) {
		AfxMessageBox(L"PRJ file not found.");
		return;
	}

	// need a working and result folder
	CString outputDir;
	m_outputFolder.GetWindowText(outputDir);

	if (outputDir.IsEmpty()) {
		AfxMessageBox(L"Output folder not set.");
		return;
	}

	std::wstring exeName = L"MvsToSLPK.exe";
	// run command line app
	std::wstring ourPath = WinUtility::executable_path();

	std::wstring folder = StdUtility::getDirectory(ourPath);

	std::wstring fullExePath = folder;
	fullExePath += exeName;

	// find installed..everything in same folder
	if (!StdUtility::fileExists(fullExePath)) {
		CString msg;
		msg = L"EXE not found.\n\n";
		msg += fullExePath.c_str();
		AfxMessageBox(msg);
		return;
	}

	int maxImageSize = _wtoi(m_maxImage);
	if (maxImageSize < 500) {
		maxImageSize = 500;
	}


	CString workingDir = outputDir;
	workingDir += "\\working";

	bool shouldDivideProject = m_radio == 1;

	ProjectDivision::ProjectDiv param;

	if (shouldDivideProject) {	
		param.filename = std::wstring(m_smtxml);
		param.workingFolder = std::wstring(workingDir);
		param.showQuadtree = true;

		param.maxImages = _wtoi(m_splitMaxImages);
		if (param.maxImages < 10) {
			param.maxImages = 10;
		}
		param.minImages = std::min(16, param.maxImages/5);

		shouldDivideProject = ProjectDivision::divideProject(param);
	}
	

	if (shouldDivideProject) {
		m_bkThread = std::thread(&CmvsToSlpkUIDlg::processMultipleProject, this, param, maxImageSize, workingDir, outputDir, fullExePath);
	}
	else {
		// make the command line
		CString cmdLine;
		cmdLine.Format(L"-s \"%ls\" -p \"%ls\" -m %d -w \"%ls\" -r \"%ls\" -g 3", m_smtxml, m_prjFile, maxImageSize, workingDir, outputDir);

		if (m_openmvsSplit) {
			cmdLine += L" -a 1";
		}

		// TEST, run colmap only
		//cmdLine += L" -j 1";

		std::wstring cmdLineStr = cmdLine;

		WinUtility::disableChildWindows(m_hWnd);
		m_cancelButton.EnableWindow(TRUE);
		m_infoStatic.EnableWindow(TRUE);

		m_threadPS = std::thread(&CmvsToSlpkUIDlg::progressServer, this);
		Sleep(500);
		m_bkThread = std::thread(&CmvsToSlpkUIDlg::processIt, this, fullExePath, cmdLineStr);
	}
}

void CmvsToSlpkUIDlg::processMultipleProject(ProjectDivision::ProjectDiv param, int maxImageSize, CString workingDir, CString outputDir, std::wstring fullExePath)
{
	stopThread = false;
	int projectNumber = 0;
	for (auto& smtxml : param.projectsGenerated) {

		if (stopThread)
			break;

		std::wstring wSmtxml = StdUtility::convert(smtxml);

		// each project needs its own working and results folder
		CString workDir = workingDir;
		std::wstring projNumStr = std::to_wstring(projectNumber).c_str();
		workDir += L"\\Proj_";
		workDir += projNumStr.c_str();

		StdUtility::createFullDirectoryPath(std::wstring(workDir));

		CString outDir = outputDir;
		outDir += L"\\Proj_";
		outDir += projNumStr.c_str();

		StdUtility::createFullDirectoryPath(std::wstring(outDir));

		// make the command line
		CString cmdLine;
		cmdLine.Format(L"-s \"%ls\" -p \"%ls\" -m %d -w \"%ls\" -r \"%ls\" -g 3", wSmtxml.c_str(), m_prjFile, maxImageSize, workDir, outDir);

		std::wstring cmdLineStr = cmdLine;
		// TEST, run colmap only
		//cmdLineStr += L" -j 1";


		WinUtility::disableChildWindows(m_hWnd);
		m_cancelButton.EnableWindow(TRUE);
		m_infoStatic.EnableWindow(TRUE);

		m_threadPS = std::thread(&CmvsToSlpkUIDlg::progressServer, this);
		Sleep(500);
		std::thread baseThread = std::thread(&CmvsToSlpkUIDlg::processItBase, this, fullExePath, cmdLineStr);

		// wait for thread to finish
		if (baseThread.joinable()) {
			baseThread.join();
		}

		endProgress();

		++projectNumber;

		if (stopThread)
			break;
	}

	PostMessage(THREAD_ENDED);
}

struct EnumWinData
{
	DWORD processId = 0;
	HWND foundWindow = nullptr;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) 
{
	EnumWinData* dataPtr = (EnumWinData*)lParam;
	DWORD processId;
	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == dataPtr->processId) {
		dataPtr->foundWindow = hwnd;
		return FALSE; // Stop enumeration
	}
	return TRUE;
}

static HWND GetConsoleWindowHandle(DWORD processId) 
{
	HWND hwnd = FindWindow(NULL, L"MvsToSLPK Terminal"); // Find Notepad window
	return hwnd;


	EnumWinData data;
	data.processId = processId;
	EnumWindows(EnumWindowsProc, (LPARAM)&data);
	return data.foundWindow;
}

void CmvsToSlpkUIDlg::processItBase(const std::wstring fullExePath, const std::wstring cmdLineStr)
{
	bool showTermial = true;
	bool waitOnIt = true;

	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;

	GetStartupInfo(&info);
	BOOL created;

	std::wstring exeFile = StdUtility::string_format(L"\"%ls\"", fullExePath.c_str());

	if (cmdLineStr.size() > 0) {
		exeFile += L" ";
		exeFile += cmdLineStr;
	}

	if (showTermial) {
		info.dwFlags = STARTF_USESHOWWINDOW;
		info.wShowWindow = SW_SHOW;
		info.lpTitle = L"MvsToSLPK Terminal";
	}

	created = CreateProcess(NULL,
		(LPWSTR)exeFile.c_str(),
		NULL,
		NULL,
		FALSE,
		showTermial ? 0 : DETACHED_PROCESS,
		//showTermial ? CREATE_NEW_PROCESS_GROUP : DETACHED_PROCESS,
		NULL,
		NULL,
		&info,
		&processInfo);


	if (!created) {
		std::wstring message = StdUtility::string_format(L"Failed to launch \"%s\"", fullExePath.c_str());

		LPTSTR lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);// Display the string.

		message += L"\n";
		message += lpMsgBuf;

		MessageBox(message.c_str(), L"GetLastError", MB_OK | MB_ICONINFORMATION);

		// Free the buffer.
		LocalFree(lpMsgBuf);

		return;
	}
	else {
		WaitForInputIdle(processInfo.hProcess, INFINITE); // Wait for console to initialize

		_consoleHandle = processInfo.hProcess;

		Sleep(550);
		HWND consoleHwnd = GetConsoleWindowHandle(processInfo.dwProcessId);

		if (consoleHwnd) {
			_consoleHwnd = consoleHwnd;
		}
		_consolePID = processInfo.dwProcessId;

		if (waitOnIt) {
			WaitForSingleObject(processInfo.hProcess, INFINITE);
		}
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		_consoleHwnd = nullptr;
		_consolePID = 0;
		_consoleHandle = nullptr;
	}
}

void CmvsToSlpkUIDlg::processIt(const std::wstring fullExePath, const std::wstring cmdLineStr)
{
	processItBase(fullExePath, cmdLineStr);

	// make sure progress ends
	endProgress();


	PostMessage(THREAD_ENDED);
}

LRESULT CmvsToSlpkUIDlg::OnThreadComplete(WPARAM wParam, LPARAM lParam)
{
	endProgress();

	if (m_bkThread.joinable()) {
		m_bkThread.join();
	}

	UpdateData(TRUE);

	WinUtility::enableChildWindows(m_hWnd);
	m_progress.SetPos(int(wParam));
	m_info = "";
	UpdateData(FALSE);

	AfxMessageBox(L"Process done.");

	return 1;
}

void CmvsToSlpkUIDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialogEx::OnOK();
}

bool CmvsToSlpkUIDlg::checkToTerminate()
{
	stopThread = true;

	// is thread running
	if (m_bkThread.joinable()) {
		HWND hwnd = ::FindWindow(NULL, L"MvsToSLPK Terminal"); // Find Notepad window
		if (hwnd) {
			::PostMessage(hwnd, WM_CLOSE, 0, 0);
			return true;
		}
	}

	return false;
}

void CmvsToSlpkUIDlg::saveToReg()
{
	UpdateData(TRUE);

	WinUtility::saveSetting(REG_KEY_NAME, L"m_radio", m_radio);
	WinUtility::saveSetting(REG_KEY_NAME, L"m_maxImage", m_maxImage);
	WinUtility::saveSetting(REG_KEY_NAME, L"m_openmvsSplit", m_openmvsSplit);
	WinUtility::saveSetting(REG_KEY_NAME, L"m_splitMaxImages", m_splitMaxImages);
}

void CmvsToSlpkUIDlg::OnCancel()
{
	if (checkToTerminate()) {
		// do nothing
	}
	else {
		saveToReg();
		CDialogEx::OnCancel();
	}
}

void CmvsToSlpkUIDlg::OnClose()
{
	if (m_bkThread.joinable()) {
		MessageBox(L"Child process is still running.  Hit cancel to stop.");
		return;
	}
	saveToReg();
	CDialogEx::OnCancel();
	//CDialogEx::OnClose();
}

void CmvsToSlpkUIDlg::endProgress()
{
	// if the server is still running this should end it
	// by sending 100 percent;
	HANDLE hPipe = nullptr;


	hPipe = CreateFile(
			TEXT("\\\\.\\pipe\\MVStoSLPK"),
			GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		if (m_threadPS.joinable()) {
			m_threadPS.join();
		}
		return;
	}


	std::string messageToSend = "100";


	DWORD bytesWritten;
	BOOL success = WriteFile(hPipe, messageToSend.c_str(), messageToSend.size(), &bytesWritten, NULL);

	CloseHandle(hPipe);

	if (m_threadPS.joinable()) {
		m_threadPS.join();
	}
}

void CmvsToSlpkUIDlg::progressServer()
{
	HANDLE hPipe;
	char buffer[1024];
	DWORD dwRead;

	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\MVStoSLPK"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, nullptr);

	if (hPipe == INVALID_HANDLE_VALUE) {
		//std::cerr << "Failed to create named pipe." << std::endl;
		return;
	}

	//std::cout << "Waiting for client connection..." << std::endl;
	ConnectNamedPipe(hPipe, nullptr);

	while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, nullptr)) {
		buffer[dwRead] = '\0';  // Null-terminate the received data
		//std::cout << "Received: " << buffer << std::endl;
		// post messages

		std::string msgStr = buffer;
		std::vector<std::string> words;
		StdUtility::tokenizeLine(msgStr, "|", words);

		int progress = 0;

		if (words.size() > 0) {
			progress = atoi(words[0].c_str());

		}

		if (words.size() > 1) {
			CString* msg = new CString;
			*msg = words[1].c_str();
			PostMessage(APP_PROGESS_PIPE, (WPARAM)progress, (LPARAM)msg);
		}

		PostMessage(APP_PROGESS_PIPE, (LPARAM)progress);

		if (progress == 100) {
			break;
		}
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
}

LRESULT CmvsToSlpkUIDlg::OnProgressPipe(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);

	m_progress.SetPos(int(wParam));

	if (lParam) {
		CString* msg = (CString*)lParam;
		m_info = *msg;
		delete msg;

		UpdateData(FALSE);
	}

	return 1;
}
