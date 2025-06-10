// WinUtility.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include <string>
#include "WinUtility.h"

#include <winerror.h>
#include <shlobj_core.h>

#include "StdUtility.h"

#include <sys\types.h> 
#include <sys\stat.h> 

#include "WinReg.hpp"
using namespace winreg;

std::wstring WinUtility::common_APPDATA(const std::wstring& location)
{
	wchar_t szPath[MAX_PATH];
	memset(szPath, 0, MAX_PATH * sizeof(wchar_t));

	bool fetchedIt = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath));

	std::wstring str = szPath;

	if (!fetchedIt || str.size() == 0) {
		// if CSIDL_COMMON_APPDATA failed, we will use the default location
		str = L"C:\\programData\\";
	}

	if (!location.empty()) {
		str += L"\\";
		str += location;
	}

	return str;
}

std::wstring WinUtility::executable_path()
{
	std::vector<wchar_t> buf(1024, 0);
	DWORD size = DWORD(buf.size());
	bool havePath = false;
	bool shouldContinue = true;
	do
	{
		DWORD result = GetModuleFileNameW(nullptr, &buf[0], size);
		DWORD lastError = GetLastError();
		if (result == 0)
		{
			shouldContinue = false;
		}
		else if (result < size)
		{
			havePath = true;
			shouldContinue = false;
		}
		else if (
			result == size
			&& (lastError == ERROR_INSUFFICIENT_BUFFER || lastError == ERROR_SUCCESS)
			)
		{
			size *= 2;
			buf.resize(size);
		}
		else
		{
			shouldContinue = false;
		}
	} while (shouldContinue);

	std::wstring ret = &buf[0];
	return ret;
}

bool WinUtility::execute(const std::wstring& executable, const std::wstring& cmdline, bool waitOnIt, bool showTermial)
{
	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;

	GetStartupInfo(&info);
	BOOL created;

	std::wstring exeFile = StdUtility::string_format(L"\"%ls\"", executable.c_str());

	if (cmdline.size() > 0) {
		exeFile += L" ";
		exeFile += cmdline;
	}

	if (showTermial) {
		info.dwFlags = STARTF_USESHOWWINDOW;
		info.wShowWindow = SW_SHOW;
	}
	
	created = CreateProcess(NULL,
		(LPWSTR)exeFile.c_str(),
		NULL,
		NULL,
		FALSE,
		showTermial ? 0 : DETACHED_PROCESS,
		NULL,
		NULL,
		&info,
		&processInfo);


	if (!created) {
			std::wstring message = StdUtility::string_format(L"Failed to launch \"%s\"", executable.c_str());

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

			MessageBox(NULL, message.c_str(), L"GetLastError", MB_OK | MB_ICONINFORMATION);

			// Free the buffer.
			LocalFree(lpMsgBuf);

		return false;
	}
	else {
		if (waitOnIt) {
			WaitForSingleObject(processInfo.hProcess, INFINITE);
		}
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return true;
}

bool WinUtility::GetEnvironmentVariable(const std::wstring & name, std::wstring& outValue)
{
	int valueLen = 16; // first shot
	outValue.resize(valueLen);
	for (;;)
	{
		int n = ::GetEnvironmentVariableW(name.c_str(), &outValue[0], valueLen);
		outValue.resize(n);
		if (n == 0)
		{
			return false;
		}
		else if (n < valueLen)
		{
			break;
		}
		else
		{
			valueLen = n;
		}
	}
	return true;
}


static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	EnableWindow(hwnd, BOOL(lParam));
	return TRUE;
}


bool WinUtility::disableChildWindows(HWND parent)
{
	return EnumChildWindows(parent, EnumChildProc, FALSE) == TRUE;
}


bool WinUtility::enableChildWindows(HWND parent)
{
	return EnumChildWindows(parent, EnumChildProc, TRUE) == TRUE;
}

static bool checkFileWriteable(const char* filename)
{
	// Just try it...
	if (filename == NULL) {
		return false;
	}

	FILE *file = 0;
	errno_t err = fopen_s(&file, filename, "wb");
	if (err != 0)
		return false;

	if (file == 0)
		return false;

	fclose(file);
	return true;
}

std::string WinUtility::getTempFilename(const char* suggestedFilename, bool overWrite /* = true */)
{
	std::string newString;
	char buff[MAX_PATH];
	memset(buff, 0, sizeof(buff));

	if (GetTempPathA(sizeof(buff), buff) == 0) {
		return newString; // Return an empty string... they'll figure it out.
	}

	std::string path = buff;
	newString = path + suggestedFilename;

	if (overWrite && checkFileWriteable(newString.c_str())) {
		return newString;
	}

	std::string fileExtension = StdUtility::getExtension(suggestedFilename);
	std::string fileBasename = StdUtility::getName(suggestedFilename, true);

	bool keepGoing = true;
	int numAttempts = 0;
	const int MAX_ATTEMPTS = 1000;
	while (keepGoing) {
		if (!StdUtility::fileExists(newString) || overWrite && checkFileWriteable(newString.c_str())) {
			return newString;
		}
		else {
			newString = StdUtility::string_format("%s%s_%d.%s", path, fileBasename, numAttempts, fileExtension);
			++numAttempts;
			keepGoing = numAttempts < MAX_ATTEMPTS;
		}
	}

	return newString;

}


static bool checkFileWriteable(const wchar_t* filename)
{
	// Just try it...
	if (filename == NULL) {
		return false;
	}

	FILE* file = 0;
	errno_t err = _wfopen_s(&file, filename, L"wb");
	if (err != 0)
		return false;

	if (file == 0)
		return false;

	fclose(file);
	return true;
}

std::wstring WinUtility::getTempFilename(const std::wstring& suggestedFilename, bool overWrite)
{
	std::wstring newString;
	wchar_t buff[MAX_PATH];
	memset(buff, 0, sizeof(buff));

	if (GetTempPathW(MAX_PATH, buff) == 0) {
		return newString; // Return an empty string... they'll figure it out.
	}

	std::wstring path = buff;
	newString = path + suggestedFilename;

	if (overWrite && checkFileWriteable(newString.c_str())) {
		return newString;
	}

	std::wstring fileExtension = StdUtility::getExtension(suggestedFilename);
	std::wstring fileBasename = StdUtility::getName(suggestedFilename, true);

	bool keepGoing = true;
	int numAttempts = 0;
	const int MAX_ATTEMPTS = 1000;
	while (keepGoing) {
		if (!StdUtility::fileExists(newString) || overWrite && checkFileWriteable(newString.c_str())) {
			return newString;
		}
		else {
			newString = StdUtility::string_format(L"%s%s_%d.%s", path, fileBasename, numAttempts, fileExtension);
			++numAttempts;
			keepGoing = numAttempts < MAX_ATTEMPTS;
		}
	}

	return newString;
}




static std::wstring REGLOCATION_KEY_NAME = L"Software\\NOT_set";

void WinUtility::setRegistryLocation(const std::wstring& loc)
{
	REGLOCATION_KEY_NAME = L"Software\\";
	REGLOCATION_KEY_NAME += loc;
}


bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::wstring& value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		key.SetStringValue(fieldName, value);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::string& value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		std::wstring wide = StdUtility::convert(value);

		key.SetStringValue(fieldName, wide);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, int value)
{
	std::wstring tmp = StdUtility::string_format(L"%d", value);
	return saveSetting(subkey, fieldName, tmp);
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, unsigned long value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		key.SetDwordValue(fieldName, value);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, const std::vector<BYTE>& data)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		key.SetBinaryValue(fieldName, data);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, _int64 value)
{
	std::wstring tmp = StdUtility::string_format(L"%I64i", value);
	return saveSetting(subkey, fieldName, tmp);
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, double value)
{
	std::wstring tmp = StdUtility::string_format(L"%f", value);
	return saveSetting(subkey, fieldName, tmp);
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, bool val)
{
	std::wstring tmp = StdUtility::string_format(L"%d", val ? 1 : 0);
	return saveSetting(subkey, fieldName, tmp);
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, const wchar_t* str)
{
	std::wstring tmp = str;
	return saveSetting(subkey, fieldName, tmp);
}

bool WinUtility::saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<std::wstring>& data)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		key.SetMultiStringValue(fieldName, data);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}


bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::wstring& value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		value = key.GetStringValue(fieldName);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::string& value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		std::wstring wide = key.GetStringValue(fieldName);
		value = StdUtility::convert(wide);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, int& value)
{
	std::wstring str;
	if (!loadSetting(subkey, fieldName, str)) {
		return false;
	}
	value = _wtoi(str.c_str());

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, unsigned long& value)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		value = key.GetDwordValue(fieldName);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<BYTE>& data)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		data = key.GetBinaryValue(fieldName);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, _int64& value)
{
	std::wstring str;
	if (!loadSetting(subkey, fieldName, str)) {
		return false;
	}
	value = _wtoi64(str.c_str());

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, double& value)
{
	std::wstring str;
	if (!loadSetting(subkey, fieldName, str)) {
		return false;
	}
	value = _wtof(str.c_str());

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, bool& val)
{
	std::wstring str;
	if (!loadSetting(subkey, fieldName, str)) {
		return false;
	}
	val = _wtoi(str.c_str()) != 0;

	return true;
}

bool WinUtility::loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<std::wstring>& data)
{
	try {
		std::wstring keyName = REGLOCATION_KEY_NAME;
		keyName += L"\\";
		keyName += subkey;

		RegKey key(HKEY_CURRENT_USER, keyName);

		data = key.GetMultiStringValue(fieldName);
	}
	catch (RegException& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}

	return true;
}
