#include "pch.h"
#include "UtilityFunc.h"
#include <fstream>
#include "StdUtil\StdUtility.h"
#include "StdUtil\WinUtility.h"
#include <iosfwd>
#include <sstream>
#include <filesystem>
#include <iostream>

bool slpkUtilityFunc::boxFile(const std::string& boxFileName, double boxData[9])
{
	FILE* fb;

	double offx = 0.0, offy = 0.0, offz = 0.0;
	double minx = 0.0, miny = 0.0, minz = 0.0;
	double maxx = 0.0, maxy = 0.0, maxz = 0.0;

	// if the box file exists, open and use it; otherwise create a new one
	if (fb = fopen(boxFileName.c_str(), "r"))
	{
		char a[2000], b[100];
		int num = 0;
		fgets(a, 2000, fb);
		num = sscanf(a, "%s %lf %lf %lf", b, &minx, &miny, &minz);
		fgets(a, 2000, fb);
		num = sscanf(a, "%s %lf %lf %lf", b, &maxx, &maxy, &maxz);
		fgets(a, 2000, fb);
		num = sscanf(a, "%s %lf %lf %lf", b, &offx, &offy, &offz);
		fclose(fb);
	}
	else {
		return false;
	}

	boxData[0] = offx, boxData[1] = offy, boxData[2] = offz;
	boxData[3] = minx, boxData[4] = miny, boxData[5] = minz;
	boxData[6] = maxx, boxData[7] = maxy, boxData[8] = maxz;

	return true;
}

std::string slpkUtilityFunc::GetProjectionWKTSTring(const std::string& prjFileName)
{
	// does nothing
	//std::setprecision(9);
	try {
		std::ifstream inFile;
		if (StdUtility::fileExists(prjFileName))
		{
			inFile.open(prjFileName);//open the input projection file
			std::stringstream strStream;
			strStream << inFile.rdbuf();//read the file
			std::string str = strStream.str();//str holds the content of the file
			return str;
		}
	}
	catch (...)
	{

	}
	return "";
}

void slpkUtilityFunc::setupProjDB()
{
	// 	static bool inited = false;
	// 	if (inited) {
	// 		return;
	// 	}
	// 	inited = true;

	// 	// adding directory of ProjDB
	// 	CHAR fullExePath[512];
	// 	DWORD numChars = GetModuleFileNameA(NULL, fullExePath, 512);
	// 	std::string dir = StdUtility::getDirectory(fullExePath, true);

#if 0
	char buff[32767];
	GetEnvironmentVariableA("Path", buff, 32766);
	std::string newPath = buff;
	newPath += ";";
	newPath += dir;
	SetEnvironmentVariableA("Path", newPath.c_str());

	SetEnvironmentVariableA("PROJ_LIB", dir.c_str());
	//SetEnvironmentVariableA("PROJ_DEBUG", "3");
	SetEnvironmentVariableA("GDAL_DATA", dir.c_str());
#endif

	// GDAL PROJ data is stored here
	//std::wstring appData = WinUtility::common_APPDATA(true, L"GDAL\\PROJ");
	std::wstring  appData = WinUtility::common_APPDATA(L"MvsToSLPK\\GDAL\\PROJ");

	// This is the only thing that seems to work
	std::filesystem::path cwdSave = std::filesystem::current_path();

	// set current path to EXE path
	std::filesystem::current_path(appData);


	bool exist = StdUtility::fileExists("proj.db");
	if (!exist) {
		std::cout << "Can't find proj.db." << std::endl;
		//printf("still cant find proj.db.\n");
	}
}
