#pragma once
#include <string>
#include <wtypes.h>
#include <vector>
#include <memory>
#include <stdexcept>
#include <regex>
// some header only string helper functions
#include "StdStringUtil.h"

namespace StdUtility
{
	// filesystem helpers
	bool fileExists(const std::string& filename);
	bool createFullDirectoryPath(const std::string& fullPath);
	uintmax_t deleteDirectory(const std::string& directory);
	bool deleteFile(const std::string& fullPath);
	bool copyFile(const std::string& srcPath, const std::string& dstPath, bool bFailIfExists = true);
	bool renameFile(const std::string& srcPath, const std::string& dstPath);
	bool deleteDirectoryContents(const std::string& srcPath);

	// regex is a regular expression, not a windows wildcard
	// example: findFiles(foundFilenames, folder, ".*\\.dll|.*\\.exe", false); This finds all dlls and exe in a folder and not any subfolders
	bool findFiles(std::vector<std::string>& foundFilenames, const std::string& search_path, const std::string& regex = ".*", bool includeSubdirs = true);
	bool makeFileReadOnly(const std::string& filename, bool makeReadOnly);
	void appendFolder(std::string& path, const std::string& folder);
	void replaceAll(std::string& source, const std::string& from, const std::string& to);

	bool fileExists(const std::wstring& filename);
	bool createFullDirectoryPath(const std::wstring& fullPath);
	uintmax_t deleteDirectory(const std::wstring& directory);
	bool deleteFile(const std::wstring& fullPath);
	bool copyFile(const std::wstring& srcPath, const std::wstring& dstPath, bool bFailIfExists = true);
	bool renameFile(const std::wstring& srcPath, const std::wstring& dstPath);
	bool deleteDirectoryContents(const std::wstring& srcPath);

	bool findFiles(std::vector<std::wstring>& foundFilenames, const std::wstring& search_path, const std::wstring& regex = L".*", bool includeSubdirs = true);
	bool findSubFoldersNamed(std::vector<std::wstring>& foundFolders, const std::wstring& search_path, const std::wstring& regex = L".*");
	bool findSubFoldersNamed(std::vector<std::string>& foundFolders, const std::string& search_path, const std::string& regex = ".*");
	bool makeFileReadOnly(const std::wstring& filename, bool makeReadOnly);
	void appendFolder(std::wstring& path, const std::wstring& folder);
	void replaceAll(std::wstring& source, const std::wstring& from, const std::wstring& to);
	

	// Path functions
	std::string getDirectory(const std::string& fullPath, bool removeSlash = false);
	std::string getName(const std::string& fullPath, bool removeExtension = true);
	std::string getExtension(const std::string& fullPath);
	std::string replaceExtension(const std::string& path, const std::string& ext);
	std::string replaceName(const std::string& path, const std::string& newName);
	// appendName and prependName  add a string to the name of the file, the path and extension are not changed
	std::string appendName(const std::string& path, const std::string& addon);
	std::string prependName(const std::string& path, const std::string& addon);
	bool hasExtension(const std::string& path, const std::string& ext);
	std::string makeValidFilename(const std::string& name);
	bool checkFileWriteable(const std::string& filename);
	std::string getTempFilename(const std::string& suggestedFilename, bool overWrite = true);
	std::string getTempFileWSubdir(const std::string& suggestedFilename, const std::string& subDir, bool overWrite = true);
	std::string getParentPath(const std::string& path);

	std::wstring getDirectory(const std::wstring& fullPath, bool removeSlash = false);
	std::wstring getName(const std::wstring& fullPath, bool removeExtension = true);
	std::wstring getExtension(const std::wstring& fullPath);
	std::wstring replaceExtension(const std::wstring& path, const std::wstring& ext);
	std::wstring replaceName(const std::wstring& path, const std::wstring& newName);
	std::wstring appendName(const std::wstring& path, const std::wstring& addon);
	std::wstring prependName(const std::wstring& path, const std::wstring& addon);
	bool hasExtension(const std::wstring& path, const std::wstring& ext);
	std::wstring  makeValidFilename(const std::wstring& name);
	bool checkFileWriteable(const std::wstring& filename);
	std::wstring getTempFilename(const std::wstring& suggestedFilename, bool overWrite = true);
	std::wstring getTempFileWSubdir(const std::wstring& suggestedFilename, const std::wstring& subDir, bool overWrite = true);
	std::wstring getParentPath(const std::wstring& path);

	// string functions
	std::wstring convert(const std::string& str);
	std::string convert(const std::wstring& str);

	std::string Right(std::string& str, int nCount);
	std::string Left(std::string& str, int nCount);
	std::string Mid(std::string& str, int iFirst, int nCount);

	std::wstring Right(std::wstring& str, int nCount);
	std::wstring Left(std::wstring& str, int nCount);
	std::wstring Mid(std::wstring& str, int iFirst, int nCount);

	int CompareNoCase(const std::string& str, const std::string& psz);
	int CompareNoCase(const std::wstring& str, const std::wstring& psz);

	std::string elapsedTimeStringA(uint32_t elaspedTime);
	std::wstring elapsedTimeString(uint32_t elaspedTime);

	std::string byteSizeStringA(uint64_t numBytes);
	std::wstring byteSizeString(uint64_t numBytes);

	void tokenizeLine(const std::string& line, const std::string& delimiters, std::vector<std::string>& words);
	void tokenizeLine(const std::wstring& line, const std::wstring& delimiters, std::vector<std::wstring>& words);
};