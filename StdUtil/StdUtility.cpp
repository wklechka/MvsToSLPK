// StdUtility.cpp : Defines the functions for the static library.
//
#include "pch.h"
#include "StdUtility.h"
#include <assert.h>

#include <filesystem>
#include <mbstring.h>
#include <string.h>

#pragma warning(disable:4996)
#include <locale>
#include <codecvt>
#include <string>

template <class TStr>
auto fileExistsT(const TStr& filename) {
	std::error_code code;
	return std::filesystem::exists(filename, code);
}

bool StdUtility::fileExists(const std::string& filename)
{
	return fileExistsT(filename);
}
bool StdUtility::fileExists(const std::wstring& filename)
{
	return fileExistsT(filename);
}

template <class TStr>
bool createFullDirectoryPathT(const TStr& fullPath)
{
	try {
		std::filesystem::path tmp = fullPath;
		std::filesystem::create_directories(tmp);
	}
	catch (std::filesystem::filesystem_error& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}
	return true;
}

bool StdUtility::createFullDirectoryPath(const std::string& fullPath)
{
	return createFullDirectoryPathT(fullPath);
}
bool StdUtility::createFullDirectoryPath(const std::wstring& fullPath)
{
	return createFullDirectoryPathT(fullPath);
}

template <class TStr>
uintmax_t deleteDirectoryT(const TStr& directory)
{
	uintmax_t numDelelted = 0;
	try {
		std::filesystem::path tmp = directory;
		numDelelted = std::filesystem::remove_all(tmp);
	}
	catch (std::filesystem::filesystem_error& e) {
		UNREFERENCED_PARAMETER(e);
		return 0;
	}
	return numDelelted;
}

uintmax_t StdUtility::deleteDirectory(const std::string& directory)
{
	return deleteDirectoryT(directory);
}
uintmax_t StdUtility::deleteDirectory(const std::wstring& directory)
{
	return deleteDirectoryT(directory);
}

template <class TStr>
bool deleteFileT(const TStr& fullPath)
{
	std::error_code ec;
	return std::filesystem::remove(fullPath, ec);
}

bool StdUtility::deleteFile(const std::string& fullPath)
{
	return deleteFileT(fullPath);
}
bool StdUtility::deleteFile(const std::wstring& fullPath)
{
	return deleteFileT(fullPath);
}

bool StdUtility::copyFile(const std::string& srcPath, const std::string& dstPath, bool bFailIfExists)
{
	try {
		std::filesystem::copy_options opt = std::filesystem::copy_options::none;
		if (!bFailIfExists)
			opt |= std::filesystem::copy_options::overwrite_existing;
		std::filesystem::copy(srcPath, dstPath, opt);
	}
	catch (std::exception&) {
		return false;
	}
	
	return true;
}
bool StdUtility::copyFile(const std::wstring& srcPath, const std::wstring& dstPath, bool bFailIfExists)
{
	try {
		std::filesystem::copy_options opt = std::filesystem::copy_options::none;
		if (!bFailIfExists)
			opt |= std::filesystem::copy_options::overwrite_existing;
		std::filesystem::copy(srcPath, dstPath);
	}
	catch (std::exception&) {
		return false;
	}
	return true;
}

template <class TStr>
bool renameFileT(TStr& srcPath, TStr& dstPath)
{
	try {
		std::filesystem::rename(srcPath, dstPath);
	}
	catch (std::exception&) {
		return false;
	}

	return true;
}

bool StdUtility::renameFile(const std::string& srcPath, const std::string& dstPath)
{
	return renameFileT(srcPath, dstPath);
}
bool StdUtility::renameFile(const std::wstring& srcPath, const std::wstring& dstPath)
{
	return renameFileT(srcPath, dstPath);
}


template <class T>
bool findFilesT(std::vector<T>& foundFilenames, const T& search_path, const T& regex, bool includeSubdirs)
{
	if (!fileExistsT(search_path))
		return false;

	if (includeSubdirs) {
		std::filesystem::path path = search_path;
		std::filesystem::recursive_directory_iterator end;
		std::filesystem::recursive_directory_iterator iter(path);

		if constexpr (std::is_convertible_v<T, std::string>) {
			const std::regex reg(regex);
			while (iter != end)
			{
				if (std::filesystem::is_regular_file(*iter)) {
					const std::string file_name = StdUtility::getName(iter->path().string(), false);
					if (std::regex_match(file_name, reg)) {
						foundFilenames.push_back(iter->path().string());
					}
				}
				++iter;
			}
		}
		else if constexpr (std::is_convertible_v<T, std::wstring>) {
			const std::wregex reg(regex);
			while (iter != end)
			{
				if (std::filesystem::is_regular_file(*iter)) {
					const std::wstring file_name = StdUtility::getName(iter->path().wstring(), false);
					if (std::regex_match(file_name, reg)) {
						foundFilenames.push_back(iter->path().wstring());
					}
				}
				++iter;
			}
		}
	}
	else {
		std::filesystem::path path = search_path;
		std::filesystem::directory_iterator  end;
		std::filesystem::directory_iterator  iter(path);

		if constexpr (std::is_convertible_v<T, std::string>) {
			const std::regex reg(regex);
			while (iter != end)
			{
				if (std::filesystem::is_regular_file(*iter)) {
					const std::string file_name = StdUtility::getName(iter->path().string(), false);
					if (std::regex_match(file_name, reg)) {
						foundFilenames.push_back(iter->path().string());
					}
				}
				++iter;
			}
		}
		else if constexpr (std::is_convertible_v<T, std::wstring>) {
			const std::wregex reg(regex);
			while (iter != end)
			{
				if (std::filesystem::is_regular_file(*iter)) {
					const std::wstring file_name = StdUtility::getName(iter->path().wstring(), false);
					if (std::regex_match(file_name, reg)) {
						foundFilenames.push_back(iter->path().wstring());
					}
				}
				++iter;
			}
		}
	}

	return foundFilenames.size() > 0;
}

bool StdUtility::findFiles(std::vector<std::wstring>& foundFilenames, const std::wstring& search_path, const std::wstring& regex, bool includeSubdirs)
{
	return findFilesT(foundFilenames, search_path, regex, includeSubdirs);
}

bool StdUtility::findFiles(std::vector<std::string>& foundFilenames, const std::string& search_path, const std::string& regex, bool includeSubdirs)
{
	return findFilesT(foundFilenames, search_path, regex, includeSubdirs);
}

bool StdUtility::findSubFoldersNamed(std::vector<std::wstring>& foundFolders, const std::wstring& search_path, const std::wstring& regex)
{
	if (!StdUtility::fileExists(search_path)) {
		return false;
	}

	try {
		std::filesystem::path path = search_path;
		std::filesystem::recursive_directory_iterator end;
		std::filesystem::recursive_directory_iterator iter(path);


		const std::wregex reg(regex);
		while (iter != end)
		{
			if (std::filesystem::is_directory(*iter)) {
				const std::wstring file_name = StdUtility::getName(iter->path().wstring(), false);
				if (std::regex_match(file_name, reg)) {
					foundFolders.push_back(iter->path().wstring());
				}
			}
			++iter;
		}	
	}
	catch (std::exception&) {
		return false;
	}

	return foundFolders.size() > 0;
}

bool StdUtility::findSubFoldersNamed(std::vector<std::string>& foundFolders, const std::string& search_path, const std::string& regex)
{
	if (!StdUtility::fileExists(search_path)) {
		return false;
	}

	try {
		std::filesystem::path path = search_path;
		std::filesystem::recursive_directory_iterator end;
		std::filesystem::recursive_directory_iterator iter(path);

		const std::regex reg(regex);
		while (iter != end)
		{
			if (std::filesystem::is_directory(*iter)) {
				const std::string file_name = StdUtility::getName(iter->path().string(), false);
				if (std::regex_match(file_name, reg)) {
					foundFolders.push_back(iter->path().string());
				}
			}
			++iter;
		}
	}
	catch (std::exception&) {
		return false;
	}

	return foundFolders.size() > 0;
}

template <class TStr>
bool makeFileReadOnlyT(const TStr& fullPath, bool makeReadOnly)
{
	std::filesystem::path path = fullPath;
	std::filesystem::perms myPerms = makeReadOnly ? std::filesystem::perms::_File_attribute_readonly : std::filesystem::perms::_All_write;
	std::error_code code;
	std::filesystem::permissions(path, myPerms, code);

	return true;
}

bool StdUtility::makeFileReadOnly(const std::string& filename, bool makeReadOnly)
{
	return makeFileReadOnlyT(filename, makeReadOnly);
}
bool StdUtility::makeFileReadOnly(const std::wstring& filename, bool makeReadOnly)
{
	return makeFileReadOnlyT(filename, makeReadOnly);
}

void StdUtility::appendFolder(std::wstring& path, const std::wstring& folder)
{
	std::filesystem::path fPath = path;
	fPath.append(folder);
	path = fPath.wstring();
}

void StdUtility::appendFolder(std::string& path, const std::string& folder)
{
	std::filesystem::path fPath = path;
	fPath.append(folder);
	path = fPath.string();
}



template <class T>
T getDirectoryT(const T& fullPath, bool removeSlash)
{
	std::filesystem::path tmp = fullPath;

	if (removeSlash) {
		tmp._Remove_filename_and_separator();
	}
	else {
		tmp.remove_filename();
	}

	if constexpr (std::is_convertible_v<T, std::string>)
		return tmp.string();
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		return tmp.wstring();
}

std::string StdUtility::getDirectory(const std::string& fullPath, bool removeSlash)
{
	return getDirectoryT(fullPath, removeSlash);
}
std::wstring StdUtility::getDirectory(const std::wstring& fullPath, bool removeSlash)
{
	return getDirectoryT(fullPath, removeSlash);
}

template <class T>
T getNameT(const T& fullPath, bool removeExtension)
{
	std::filesystem::path tmp = fullPath;

	if (removeExtension) {
		if constexpr (std::is_convertible_v<T, std::string>)
			return tmp.stem().string();
		else if constexpr (std::is_convertible_v<T, std::wstring>)
			return tmp.stem().wstring();
	}

	if constexpr (std::is_convertible_v<T, std::string>)
		return tmp.filename().string();
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		return tmp.filename().wstring();
}

std::string StdUtility::getName(const std::string& fullPath, bool removeExtension)
{
	return getNameT(fullPath, removeExtension);
}
std::wstring StdUtility::getName(const std::wstring& fullPath, bool removeExtension)
{
	return getNameT(fullPath, removeExtension);
}


template <class T>
T getExtensionT(const T& fullPath)
{
	std::filesystem::path tmp = fullPath;

	T extension;
	if constexpr (std::is_convertible_v<T, std::string>)
		extension = tmp.extension().string();
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		extension = tmp.extension().wstring();


	if (extension.size() > 0 && extension[0] == '.') {
		extension.erase(extension.begin());
	}

	return extension;
}

std::string StdUtility::getExtension(const std::string& fullPath)
{
	return getExtensionT(fullPath);
}
std::wstring StdUtility::getExtension(const std::wstring& fullPath)
{
	return getExtensionT(fullPath);
}

template <class T>
bool hasExtensionT(const T& fullPath, const T& ext)
{
	std::filesystem::path tmp = fullPath;

	T extension = getExtensionT(fullPath);

	int result = StdUtility::CompareNoCase(extension, ext);

	return result == 0;
}

bool StdUtility::hasExtension(const std::string& path, const std::string& ext)
{
	return hasExtensionT(path, ext);
}

bool StdUtility::hasExtension(const std::wstring& path, const std::wstring& ext)
{
	return hasExtensionT(path, ext);
}

std::string StdUtility::makeValidFilename(const std::string& name)
{
	return std::regex_replace(name, std::regex("[:\\*\\?\"<>\\|/\\\\]"), "_");;
}
std::wstring  StdUtility::makeValidFilename(const std::wstring& name)
{
	return std::regex_replace(name, std::wregex(L"[:\\*\\?\"<>\\|/\\\\]"), L"_");
}

bool StdUtility::checkFileWriteable(const std::string& filename)
{
	if (!fileExists(filename)) {
		// well it don't exist so it must be writable
		return true;
	}

	FILE* file_handle;
	errno_t file_open_error;
	if ((file_open_error = fopen_s(&file_handle, filename.c_str(), "a")) != 0)
	{
		return false;
	}

	fclose(file_handle);
	return true;
}
bool StdUtility::checkFileWriteable(const std::wstring& filename)
{
	std::string tmpStr = convert(filename);
	return checkFileWriteable(tmpStr);
}

std::string getTempFilenamePriv(const std::filesystem::path &path, const std::string& suggestedFilename, bool overWrite)
{
	auto newString = path;
	newString.append(suggestedFilename);

	if (overWrite && StdUtility::checkFileWriteable(newString.string())) {
		return newString.string();
	}

	std::string fileExtension = StdUtility::getExtension(suggestedFilename);
	std::string fileBasename = StdUtility::getName(suggestedFilename, true);

	bool keepGoing = true;
	int numAttempts = 0;
	const int MAX_ATTEMPTS = 1000;
	while (keepGoing) {
		if (!StdUtility::fileExists(newString) || overWrite && StdUtility::checkFileWriteable(newString.string())) {
			return newString.string();
		}
		else {
			std::string strPath = path.string();
			
			if (strPath.size() > 0) 
			{
				if (!(strPath[strPath.size() - 1] == '\\' ||
					strPath[strPath.size() - 1] == '/')) {
					strPath += "\\";
				}
			}
	
			std::string result = StdUtility::string_format("%s%s_%d.%s", strPath.c_str(), fileBasename.c_str(), numAttempts, fileExtension.c_str());

			newString = result;

			++numAttempts;
			keepGoing = numAttempts < MAX_ATTEMPTS;
		}
	}

	return newString.string();
}

std::string StdUtility::getTempFilename(const std::string& suggestedFilename, bool overWrite)
{
	auto path = std::filesystem::temp_directory_path();
	return getTempFilenamePriv(path, suggestedFilename, overWrite);
}
std::wstring StdUtility::getTempFilename(const std::wstring& suggestedFilename, bool overWrite)
{
	std::string tmpStr = convert(suggestedFilename);
	std::string result = getTempFilename(tmpStr, overWrite);
	return convert(result);
}


std::string StdUtility::getTempFileWSubdir(const std::string& suggestedFilename, const std::string& subDir, bool overWrite)
{
	auto path = std::filesystem::temp_directory_path();
	if (subDir.size() > 0) {
		path.append(subDir);
		createFullDirectoryPath(path.string());
	}
	return getTempFilenamePriv(path, suggestedFilename, overWrite);
}
std::wstring StdUtility::getTempFileWSubdir(const std::wstring& suggestedFilename, const std::wstring& subDir, bool overWrite)
{
	std::string tmpStr = convert(suggestedFilename);
	std::string tmpStrDir = convert(subDir);
	std::string result = getTempFileWSubdir(tmpStr, tmpStrDir, overWrite);
	return convert(result);
}


std::string StdUtility::replaceExtension(const std::string& path, const std::string& ext)
{
	return std::filesystem::path(path).replace_extension(ext).string();
}
std::wstring StdUtility::replaceExtension(const std::wstring& path, const std::wstring& ext)
{
	return std::filesystem::path(path).replace_extension(ext).wstring();
}

std::string StdUtility::replaceName(const std::string& path, const std::string& newName)
{
	// if there is not a new extension then use the old one
	if (getExtension(newName).size() == 0) {
		std::filesystem::path tmp = path;
		std::filesystem::path ext = tmp.extension();
		std::string returnVal = tmp.replace_filename(makeValidFilename(newName)).string();
		returnVal.append(ext.string());
		return returnVal;
	}
	return std::filesystem::path(path).replace_filename(makeValidFilename(newName)).string();
}
std::wstring StdUtility::replaceName(const std::wstring& path, const std::wstring& newName)
{
	// if there is not a new extension then use the old one
	if (getExtension(newName).size() == 0) {
		std::filesystem::path tmp = path;
		std::filesystem::path ext = tmp.extension();
		std::wstring returnVal = tmp.replace_filename(makeValidFilename(newName)).wstring();
		returnVal.append(ext.wstring());
		return returnVal;
	}
	return std::filesystem::path(path).replace_filename(makeValidFilename(newName)).wstring();
}

template <class T>
T appendNameT(const T& path, const T& addon)
{
	T newName = StdUtility::getName(path);
	newName += addon;

	if constexpr (std::is_convertible_v<T, std::string>)
		newName += ".";
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		newName += L".";

	newName += StdUtility::getExtension(path);


	if constexpr (std::is_convertible_v<T, std::string>)
		return std::filesystem::path(path).replace_filename(newName).string();
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		return std::filesystem::path(path).replace_filename(newName).wstring();
}

std::string StdUtility::appendName(const std::string& path, const std::string& addon)
{
	return appendNameT(path, addon);
}
std::wstring StdUtility::appendName(const std::wstring& path, const std::wstring& addon)
{
	return appendNameT(path, addon);
}


template <class T>
T  prependNameT(const T& path, const T& addon)
{
	T newName = addon;
	newName += StdUtility::getName(path, false);

	if constexpr (std::is_convertible_v<T, std::string>)
		return std::filesystem::path(path).replace_filename(newName).string();
	else if constexpr (std::is_convertible_v<T, std::wstring>)
		return std::filesystem::path(path).replace_filename(newName).wstring();
}

std::string StdUtility::prependName(const std::string& path, const std::string& addon)
{
	return prependNameT(path, addon);
}
std::wstring StdUtility::prependName(const std::wstring& path, const std::wstring& addon)
{
	return prependNameT(path, addon);
}


template <class T>
T RightT(const T& str, int nCount)
{
	if (nCount < 0)
		nCount = 0;

	std::size_t nLength = str.size();
	if (nCount >= nLength)
	{
		return str;
	}
	auto iterStart = str.begin() + nLength - nCount;
	auto iterEnd = iterStart + nCount;
	return(T(iterStart, iterEnd));
}


std::string StdUtility::Right(std::string& str, int nCount)
{
	return RightT(str, nCount);
}
std::wstring StdUtility::Right(std::wstring& str, int nCount)
{
	return RightT(str, nCount);
}

template <class T>
T LeftT(const T& str, int nCount)
{
	if (nCount < 0)
		nCount = 0;

	std::size_t nLength = str.size();
	if (nCount >= nLength)
	{
		return(str);
	}

	return T(str.begin(), str.begin() + nCount);
}

std::string StdUtility::Left(std::string& str, int nCount)
{
	return LeftT(str, nCount);
}
std::wstring StdUtility::Left(std::wstring& str, int nCount)
{
	return LeftT(str, nCount);
}

template <class T>
T MidT(const T& str, int iFirst, int nCount)
{
	if (iFirst < 0)
		iFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if ((iFirst + nCount) > str.size())
	{
		nCount = int(str.size()) - iFirst;
	}
	if (iFirst > str.size())
	{
		nCount = 0;
	}

	assert((nCount == 0) || ((iFirst + nCount) <= str.size()));

	if ((iFirst == 0) && ((iFirst + nCount) == str.size()))
	{
		return(str);
	}

	return T(str.begin() + iFirst, str.begin() + iFirst + nCount);
}

std::string StdUtility::Mid(std::string& str, int iFirst, int nCount)
{
	return MidT(str, iFirst, nCount);
}
std::wstring StdUtility::Mid(std::wstring& str, int iFirst, int nCount)
{
	return MidT(str, iFirst, nCount);
}

int StdUtility::CompareNoCase(const std::string& str, const std::string& psz)
{
	return lstrcmpiA((LPCSTR)str.c_str(), (LPCSTR)psz.c_str());
}
int StdUtility::CompareNoCase(const std::wstring& str, const std::wstring& psz)
{
	return lstrcmpiW((LPCWSTR)str.c_str(), (LPCWSTR)psz.c_str());
}


std::wstring  StdUtility::convert(const std::string& str)
{
	std::wstring result;

	wchar_t* mbname = NULL;
	int mbsize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	if (mbsize > 0) {
		mbname = new wchar_t[mbsize];
		if (!mbname) {
			return std::wstring();
		}

		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, mbname, mbsize);

		result = mbname;
		delete mbname;
	}
	return result;
}

std::string  StdUtility::convert(const std::wstring& str)
{
	std::string result;

	char* mbname = NULL;
	int mbsize = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	if (mbsize > 0) {
		mbname = new char[mbsize];
		if (!mbname) {
			return std::string();
		}

		WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, mbname, mbsize,
			NULL, NULL);

		result = mbname;
		delete mbname;
	}
	return result;
}

static void millisecondTo(uint32_t milliseconds, int& hours, int& min, int& seconds)
{
	int totalseconds = milliseconds / 1000;
	hours = totalseconds / 3600;
	totalseconds -= hours * 3600;
	min = totalseconds / 60;
	totalseconds -= min * 60;
	seconds = totalseconds;
}

template <class T>
T elapsedTimeStringT(uint32_t elaspedTime)
{
	int hours, minutes, seconds;
	millisecondTo(elaspedTime, hours, minutes, seconds);

	T tmpStr;
	if (hours > 0) {
		if constexpr (std::is_convertible_v<T, std::string>)
			tmpStr = StdUtility::string_format("hours: %d, minutes: %d, seconds: %d", hours, minutes, seconds);
		else if constexpr (std::is_convertible_v<T, std::wstring>)
			tmpStr = StdUtility::string_format(L"hours: %d, minutes: %d, seconds: %d", hours, minutes, seconds);

	}
	else {
		if constexpr (std::is_convertible_v<T, std::string>)
			tmpStr = StdUtility::string_format("minutes: % d, seconds : % d", minutes, seconds);
		else if constexpr (std::is_convertible_v<T, std::wstring>)
			tmpStr = StdUtility::string_format(L"minutes: % d, seconds : % d", minutes, seconds);
	}

	return tmpStr;
}

std::string StdUtility::elapsedTimeStringA(uint32_t elaspedTime)
{
	return elapsedTimeStringT<std::string>(elaspedTime);
}
std::wstring StdUtility::elapsedTimeString(uint32_t elaspedTime)
{
	return elapsedTimeStringT<std::wstring>(elaspedTime);
}

std::string StdUtility::byteSizeStringA(uint64_t numBytes)
{
	double dKSize = 1024.0;

	int unitIndex = 0;
	const char units[][6] = { "Bytes", "KB", "MB", "GB", "TB", "PB" };

	double dFileSize = double(numBytes);

	while (dFileSize > dKSize && unitIndex <= 5) {
		dFileSize /= dKSize;
		++unitIndex;
	}

	std::string tmp = StdUtility::string_format("%.2f ", dFileSize);
	tmp += units[unitIndex];

	return tmp;
}

std::wstring StdUtility::byteSizeString(uint64_t numBytes)
{
	std::string tmp = byteSizeStringA(numBytes);
	return convert(tmp);
}


void StdUtility::tokenizeLine(const std::string& line, const std::string& delimiters, std::vector<std::string>& words)
{
	std::string regString = "[^";
	regString += delimiters;
	regString += "]+(?=";
	regString += delimiters;
	regString += "|$)";

	//std::regex reg("[^|]+(?=|)"); // delimiter of |
	std::regex reg(regString);

	auto words_begin = std::sregex_iterator(line.begin(), line.end(), reg);
	auto words_end = std::sregex_iterator();

	for (auto iter = words_begin; iter != words_end; ++iter) {
		words.push_back(iter->str());
	}
}

void StdUtility::tokenizeLine(const std::wstring& line, const std::wstring& delimiters, std::vector<std::wstring>& words)
{
	std::wstring regString = L"[^";
	regString += delimiters;
	regString += L"]+(?=";
	regString += delimiters;
	regString += L"|$)";

	//std::regex reg("[^|]+(?=|)"); // delimiter of |
	std::wregex reg(regString);

	auto words_begin = std::wsregex_iterator(line.begin(), line.end(), reg);
	auto words_end = std::wsregex_iterator();

	for (auto iter = words_begin; iter != words_end; ++iter) {
		words.push_back(iter->str());
	}
}

template <class TStr>
void replaceAllT(TStr& source, const TStr& from, const TStr& to)
{
	TStr newString;
	newString.reserve(source.length());

	if constexpr (std::is_convertible_v<TStr, std::string>) {
		std::string::size_type lastPos = 0;
		std::string::size_type findPos;

		while (std::string::npos != (findPos = source.find(from, lastPos)))
		{
			newString.append(source, lastPos, findPos - lastPos);
			newString += to;
			lastPos = findPos + from.length();
		}
		// Care for the rest after last occurrence
		newString += source.substr(lastPos);
	}
	else if constexpr (std::is_convertible_v<TStr, std::wstring>) {
		std::wstring::size_type lastPos = 0;
		std::wstring::size_type findPos;

		while (std::wstring::npos != (findPos = source.find(from, lastPos)))
		{
			newString.append(source, lastPos, findPos - lastPos);
			newString += to;
			lastPos = findPos + from.length();
		}

		// Care for the rest after last occurrence
		newString += source.substr(lastPos);
	}

	source.swap(newString);
}

void StdUtility::replaceAll(std::wstring& source, const std::wstring& from, const std::wstring& to)
{
	replaceAllT(source, from, to);
}
void StdUtility::replaceAll(std::string& source, const std::string& from, const std::string& to)
{
	replaceAllT(source, from, to);
}

template <class TStr>
bool deleteDirectoryContentsT(const TStr& fullPath)
{
	try {
		std::filesystem::path dir = fullPath;

		for (const auto& entry : std::filesystem::directory_iterator(dir))
			std::filesystem::remove_all(entry.path());
	}
	catch (std::filesystem::filesystem_error& e) {
		UNREFERENCED_PARAMETER(e);
		return false;
	}
	return true;
}

bool StdUtility::deleteDirectoryContents(const std::string& srcPath)
{
	return deleteDirectoryContentsT(srcPath);
}
bool StdUtility::deleteDirectoryContents(const std::wstring& srcPath)
{
	return deleteDirectoryContentsT(srcPath);
}



std::string StdUtility::getParentPath(const std::string& path)
{
	std::filesystem::path mosPath = path;
	return mosPath.parent_path().string();
}
std::wstring StdUtility::getParentPath(const std::wstring& path)
{
	std::filesystem::path mosPath = path;
	return mosPath.parent_path().wstring();
}
