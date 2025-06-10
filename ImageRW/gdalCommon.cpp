#include "pch.h"
#include "gdalCommon.h"
#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "gdal.h"
#include "gdal_priv.h"
#include <fstream>
#include <stack>
#include <algorithm>
#include <string>
#include <mutex>
#include "StdUtil/StdUtility.h"
#include "StdUtil/WinUtility.h"

const TCHAR* PROJDB = L"proj.db";
const TCHAR* PROJDB_ENV = L"PROJ_DATA=";
const TCHAR* GDAL_ENV = L"GDAL_DATA=";

void findandSetProjDB()
{
	std::wstring filename;

	// try ProgramData, really it must be here
	//std::wstring  lastDir = WinUtility::common_APPDATA(true, L"GDAL");
	std::wstring  lastDir = WinUtility::common_APPDATA(L"MvsToSLPK\\GDAL");

	filename = lastDir;
	filename += L"\\PROJ\\";
	filename += PROJDB;

	if (StdUtility::fileExists(filename)) {
		std::wstring envVal = PROJDB_ENV;
		envVal += StdUtility::getDirectory(filename, true);

		_putenv(StdUtility::convert(envVal).c_str());

		std::wstring gdalEnvVal = GDAL_ENV;
		gdalEnvVal += lastDir;
		_putenv(StdUtility::convert(gdalEnvVal).c_str());
		return;
	}
}

static std::mutex initMutex;

void initGDAL()
{
	initMutex.lock();
	static bool doOnce = true;
	if (doOnce) {
		findandSetProjDB();
		GDALAllRegister();
		CPLSetConfigOption("GTIFF_POINT_GEO_IGNORE", "TRUE");
		doOnce = false;
	}
	initMutex.unlock();
}
