#pragma once
#include <windows.h>  
#include <string>
#include <vector>

namespace WinUtility
{
	std::wstring common_APPDATA(const std::wstring& location);

	std::wstring executable_path();

	bool execute(const std::wstring& executable, const std::wstring& cmdline, bool waitOnIt = false, bool showTermial = false);
	
	bool GetEnvironmentVariable(const std::wstring& name, std::wstring& outValue);

	bool disableChildWindows(HWND parent);
	bool enableChildWindows(HWND parent);

	std::string getTempFilename(const char* suggestedFilename, bool overWrite = true);
	std::wstring getTempFilename(const std::wstring &suggestedFilename, bool overWrite = true);


	void setRegistryLocation(const std::wstring& loc);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::wstring& value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::string& value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, int value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, unsigned long value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, const std::vector<BYTE>& data);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, _int64 value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, double value);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<std::wstring>& data);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, bool val);
	bool saveSetting(const std::wstring& subkey, const std::wstring& fieldName, const wchar_t* str);

	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::wstring& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::string& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, int& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, unsigned long& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<BYTE>& data);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, _int64& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, double& value);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, std::vector<std::wstring>& data);
	bool loadSetting(const std::wstring& subkey, const std::wstring& fieldName, bool& val);
};
