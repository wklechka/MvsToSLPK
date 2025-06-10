#include <windows.h>
#include "MvsCommon.h"
#include <string>
#include <fstream>
#include <iostream>
#include <iosfwd>
#include <iomanip>
#include <map>

#include "StdUtil/StdUtility.h"

#include "ImageRW/IImageReader.h"
#include "ImageRW/IImageWriter.h"


#include "PTFile/IPointFile.h"

#include <datetimeapi.h>

#include "Eigen/Geometry"
#include <spdlog/spdlog.h>

#include "sqlite3.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "happly/happly.h"

#include <thread>
#include <atomic>


DWORD execute(const std::wstring& executable, const std::wstring& cmdline)
{
	DWORD exitCode = -1;

	bool showError = true;
	bool waitOnIt = true;

	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;

	GetStartupInfo(&info);
	BOOL created;

	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_SHOW;

	std::wstring exeFile = StdUtility::string_format(L"\"%ls\"", executable.c_str());

	if (cmdline.size() > 0) {
		exeFile += L" ";
		exeFile += cmdline;
	}

	created = CreateProcess(NULL,
		(LPWSTR)exeFile.c_str(),
		NULL,
		NULL,
		FALSE,
		0, // must be 0 to show the window
		NULL,
		NULL,
		&info,
		&processInfo);


	if (!created) {
		if (showError) {
			std::string message = StdUtility::string_format("Failed to launch \"%s\"", executable.c_str());
			spdlog::error(message);
		}

		return exitCode;
	}
	else {
		if (waitOnIt) {
			WaitForSingleObject(processInfo.hProcess, INFINITE);

			GetExitCodeProcess(processInfo.hProcess, &exitCode);
		}
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return exitCode;
}


static std::atomic<bool> stopExecution = false;
static void pipeServer(HANDLE hPipe)
{
	char buffer[1024];
	DWORD dwRead;

	while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, nullptr)) {

		buffer[dwRead] = '\0';  // Null-terminate the received data
		// post messages
		std::string outString = buffer;
		StdUtility::replaceAll(outString, "\r\n", "\n");

		std::cout << outString.c_str();

		if (stopExecution) {
			break;
		}
	}
}

// a special execute that sends the output to a pipe
// This is done so that we can log the output of the process to a file and also display it in the console.
// this works with COLMAP, but not with OpenMVS
// OpenMVS already logs to a file, so this is not needed for OpenMVS. Also, OpenMVS does not use the console output in the same way as COLMAP.
DWORD executeColmap(const std::wstring& executable, const std::wstring& cmdline)
{
	DWORD exitCode = -1;

	bool showError = true;
	bool waitOnIt = true;

	STARTUPINFO info;
	PROCESS_INFORMATION processInfo;

	GetStartupInfo(&info);
	BOOL created;

 	info.dwFlags = STARTF_USESHOWWINDOW;
 	info.wShowWindow = SW_SHOW;

	HANDLE hRead, hWrite;
	SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
	saAttr.bInheritHandle = TRUE; // Allow child processes to inherit the handles

	if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
		std::cerr << "Failed to create pipe.\n";
		return 1;
	}
	SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	stopExecution = false;
	std::thread threadPipe = std::thread(&pipeServer, hRead);

	DWORD bytesWritten;
	std::string messageToSend = "\n";
	BOOL success = WriteFile(hWrite, messageToSend.c_str(), strlen(messageToSend.c_str()), &bytesWritten, NULL);


	info.hStdOutput = hWrite;
	info.hStdError = hWrite;
	info.dwFlags = STARTF_USESTDHANDLES;

	//CloseHandle(hWrite);

	std::wstring exeFile = StdUtility::string_format(L"\"%ls\"", executable.c_str());

	if (cmdline.size() > 0) {
		exeFile += L" ";
		exeFile += cmdline;
	}

	created = CreateProcess(NULL,
		(LPWSTR)exeFile.c_str(),
		NULL,
		NULL,
		TRUE,
		0, // must be 0 to show the window
		NULL,
		NULL,
		&info,
		&processInfo);

	if (!created) {
		if (showError) {
			std::string message = StdUtility::string_format("Failed to launch \"%s\"", executable.c_str());
			spdlog::error(message);
		}

		return exitCode;
	}
	else {
		if (waitOnIt) {
			WaitForSingleObject(processInfo.hProcess, INFINITE);

			GetExitCodeProcess(processInfo.hProcess, &exitCode);
		}
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	// stop pipe reading thread
	stopExecution = true;
	std::string messageToSend2 = "\n";
	BOOL success2 = WriteFile(hWrite, messageToSend2.c_str(), strlen(messageToSend2.c_str()), &bytesWritten, NULL);

	
	threadPipe.join();
	CloseHandle(hRead);
	CloseHandle(hWrite);

	return exitCode;
}


bool readBoxFile(const std::string& boxFileName, double boxData[9])
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


struct WriterWData
{
	IWritePT *iw;
	double offsetX, offsetY, offsetZ;
};

static bool readCallbackWriteIt(IReadPointData& pointData, void* clientData)
{
	WriterWData* writer = (WriterWData*)clientData;
	writer->iw->addPt(pointData.x + writer->offsetX, pointData.y + writer->offsetY, pointData.z + writer->offsetZ, pointData.color, pointData.intensity);

	return true;
}

struct PointStats
{
	double minX, minY, minZ;
	double maxX, maxY, maxZ;
};
static bool readCallbackExtents(IReadPointData& pointData, void* clientData)
{
	PointStats* ps = (PointStats*)clientData;

	ps->minX = std::min(ps->minX, pointData.x);
	ps->minY = std::min(ps->minY, pointData.y);
	ps->minZ = std::min(ps->minZ, pointData.z);

	ps->maxX = std::max(ps->maxX, pointData.x);
	ps->maxY = std::max(ps->maxY, pointData.y);
	ps->maxZ = std::max(ps->maxZ, pointData.z);

	return true;
}

bool PlyFileToLAS(std::vector<std::wstring>& densePtsPlyResults, const std::string& outLasFile, const std::string& boxFilename)
{
	return false;

	double boxData[9];
	readBoxFile(boxFilename, boxData);

	auto ptReader = std::shared_ptr<IReadPT>(createIReadPT(), [](IReadPT* p) { p->Release(); });
	auto ptWriter = std::shared_ptr<IWritePT>(createIWritePT(), [](IWritePT* p) { p->Release(); });

	// read hold file to get extents
	PointStats ps;
	ps.minX = FLT_MAX;
	ps.minY = FLT_MAX;
	ps.minZ = FLT_MAX;

	ps.maxX = -FLT_MAX;
	ps.maxY = -FLT_MAX;
	ps.maxZ = -FLT_MAX;

	// write the new file...
	for (auto& plyFile : densePtsPlyResults) {
		std::string firstFile = StdUtility::convert(plyFile);
		ptReader->addFilename(firstFile.c_str());
	}

	ptReader->readPoints(readCallbackExtents, &ps);

	WriterWData wd;
	wd.iw = ptWriter.get();
	wd.offsetX = boxData[0];
	wd.offsetY = boxData[1];
	wd.offsetZ = boxData[2];

	ps.minX += wd.offsetX;
	ps.minY += wd.offsetY;
	ps.minZ += wd.offsetZ;

	ps.maxX += wd.offsetX;
	ps.maxY += wd.offsetY;
	ps.maxZ += wd.offsetZ;

	ptWriter->startWrite(outLasFile.c_str(), ps.minX, ps.minY, ps.minZ, ps.maxX, ps.maxY, ps.maxZ);
	ptReader->readPoints(readCallbackWriteIt, &wd);
	ptWriter->finishWrite();

	return true;
}

bool PlyFileToLAS(const std::string& firstFile, const std::string& secondFile, const std::string& boxFilename)
{
	return false;

	double boxData[9];
	readBoxFile(boxFilename, boxData);

	auto ptReader = std::shared_ptr<IReadPT>(createIReadPT(), [](IReadPT* p) { p->Release(); });
	auto ptWriter = std::shared_ptr<IWritePT>(createIWritePT(), [](IWritePT* p) { p->Release(); });

	// read hold file to get extents
	PointStats ps;
	ps.minX = FLT_MAX;
	ps.minY = FLT_MAX;
	ps.minZ = FLT_MAX;

	ps.maxX = -FLT_MAX;
	ps.maxY = -FLT_MAX;
	ps.maxZ = -FLT_MAX;

	// write the new file...
	ptReader->addFilename(firstFile.c_str());
	ptReader->readPoints(readCallbackExtents, &ps);

	WriterWData wd;
	wd.iw = ptWriter.get();
	wd.offsetX = boxData[0];
	wd.offsetY = boxData[1];
	wd.offsetZ = boxData[2];

	ps.minX += wd.offsetX;
	ps.minY += wd.offsetY;
	ps.minZ += wd.offsetZ;

	ps.maxX += wd.offsetX;
	ps.maxY += wd.offsetY;
	ps.maxZ += wd.offsetZ;

	ptWriter->startWrite(secondFile.c_str(), ps.minX, ps.minY, ps.minZ, ps.maxX, ps.maxY, ps.maxZ);
	ptReader->readPoints(readCallbackWriteIt, &wd);
	ptWriter->finishWrite();

	return true;
}

bool writeBoxFile(const std::string& boxFilename, const Eigen::Vector3d& offsetPt, const Eigen::Vector3d& bbMin, const Eigen::Vector3d& bbMax)
{
 	std::ofstream outFile;
	outFile.open(boxFilename);
	if (!outFile.is_open()) {
		return false;
	}

	std::string line1 = StdUtility::string_format("bb_min_f %.3f %.3f %.3f", bbMin[0], bbMin[1], bbMin[2]);
	std::string line2 = StdUtility::string_format("bb_max_f %.3f %.3f %.3f", bbMax[0], bbMax[1], bbMax[2]);
	std::string line3 = StdUtility::string_format("offset %.3f %.3f %.3f", offsetPt[0], offsetPt[1], offsetPt[2]);

	outFile << line1 << std::endl;
	outFile << line2 << std::endl;
	outFile << line3 << std::endl;

	outFile.close();

	return true;
}


bool resizeTexturePow2(const std::string& firstFile)
{
	auto iFormat = std::shared_ptr<IImageReader>(createIImageReader(), [](IImageReader* p) { p->Release(); });

	iFormat->ignoreUpperBands(true);
	if (!iFormat->setFile(firstFile.c_str())) {
		return false;
	}

	GDALImageInfo info;
	if (!iFormat->getImageInfo(info)) {
		return false;
	}

	unsigned int textureSizeLimit = 16384;

	if (info._imageWidth <= textureSizeLimit && info._imageHeight <= textureSizeLimit) {
		// no processing need
		return true;
	}

	std::cout << "resizeTexturePow2: texture resizing." << std::endl;

	// texture too big ... reduce to 
	uint8_t* dataToFill = new uint8_t[iFormat->readRectMemSize(textureSizeLimit, textureSizeLimit)];
	iFormat->readRect(0, 0, info._imageWidth, info._imageHeight, textureSizeLimit, textureSizeLimit, dataToFill);
	
	std::string tmpFilename = StdUtility::appendName(firstFile, "_tmp");
	tmpFilename = StdUtility::replaceExtension(tmpFilename, "tif");

	auto writer = std::shared_ptr<IImageWriter>(createIImageWriter(), [](IImageWriter* p) { p->Release(); });

	writer->writeAsTiff(dataToFill, textureSizeLimit, textureSizeLimit, tmpFilename.c_str(), 3);

	iFormat.reset();

	std::string tooBigFilename = StdUtility::appendName(firstFile, "_tooBig");
	StdUtility::renameFile(firstFile, tooBigFilename);

	std::string destFilename = firstFile;

	auto gdal = std::shared_ptr<IImageWriter>(createIImageWriter(), [](IImageWriter* p) { p->Release(); });
	gdal->createCopy(tmpFilename.c_str(), destFilename.c_str(), nullptr, "PNG");

	StdUtility::deleteFile(tmpFilename);

	delete [] dataToFill;

	return true;
}

struct ColmapPointData
{
	double x, y, z;
	uint32_t color;
	unsigned short intensity = 0;
};

typedef bool(*ColmapPointDataCallback)(ColmapPointData& pointData, void* clientData);

static const int BUFF_SIZE = 1000;

class ColmapPtReader
{
public:

	bool setFile(const std::string& ptFilename)
	{
		_ptFilename = ptFilename;
		return true;
	}
	bool readPoints(ColmapPointDataCallback callback, void* clientData)
	{
		std::ifstream	file;
		file.open(_ptFilename);

		if (!file.is_open()) {
			return false;
		}
		char	buff[BUFF_SIZE + 1] = "";

		while (file.getline(buff, BUFF_SIZE).good()) {
			std::string lineData = buff;
			std::vector<std::string> words;
			StdUtility::tokenizeLine(lineData, " ", words);

			if (words.size() < 7)
				continue;

			// check of comment
			if (words[0][0] == '#')
				continue;

			// actual data
			ColmapPointData ptData;
			ptData.x = atof(words[1].c_str());
			ptData.y = atof(words[2].c_str());
			ptData.z = atof(words[3].c_str());
			ptData.color = RGB(atoi(words[4].c_str()), atoi(words[5].c_str()), atoi(words[6].c_str()));
			callback(ptData, clientData);
		}
		file.close();
		return true;
	}

protected:
	std::string _ptFilename;
};


static bool readColmapExtents(ColmapPointData& pointData, void* clientData)
{
	PointStats* ps = (PointStats*)clientData;

	ps->minX = std::min(ps->minX, pointData.x);
	ps->minY = std::min(ps->minY, pointData.y);
	ps->minZ = std::min(ps->minZ, pointData.z);

	ps->maxX = std::max(ps->maxX, pointData.x);
	ps->maxY = std::max(ps->maxY, pointData.y);
	ps->maxZ = std::max(ps->maxZ, pointData.z);

	return true;
}

static bool readCallbackWriteIt(ColmapPointData& pointData, void* clientData)
{
	WriterWData* writer = (WriterWData*)clientData;
	writer->iw->addPt(pointData.x + writer->offsetX, pointData.y + writer->offsetY, pointData.z + writer->offsetZ, pointData.color, pointData.intensity);

	return true;
}

bool colmapPtsToLas(const std::string& boxFilename, const std::string& ptFilename, const std::string& lasFilename)
{
	double boxData[9];
	readBoxFile(boxFilename, boxData);

	ColmapPtReader colReader;

	auto ptWriter = std::shared_ptr<IWritePT>(createIWritePT(), [](IWritePT* p) { p->Release(); });

	// read hold file to get extents
	PointStats ps;
	ps.minX = FLT_MAX;
	ps.minY = FLT_MAX;
	ps.minZ = FLT_MAX;

	ps.maxX = -FLT_MAX;
	ps.maxY = -FLT_MAX;
	ps.maxZ = -FLT_MAX;

	// write the new file...
	colReader.setFile(ptFilename);
	colReader.readPoints(readColmapExtents, &ps);

	WriterWData wd;
	wd.iw = ptWriter.get();
	wd.offsetX = boxData[0];
	wd.offsetY = boxData[1];
	wd.offsetZ = boxData[2];

	ps.minX += wd.offsetX;
	ps.minY += wd.offsetY;
	ps.minZ += wd.offsetZ;

	ps.maxX += wd.offsetX;
	ps.maxY += wd.offsetY;
	ps.maxZ += wd.offsetZ;

	ptWriter->startWrite(lasFilename.c_str(), ps.minX, ps.minY, ps.minZ, ps.maxX, ps.maxY, ps.maxZ);
	colReader.readPoints(readCallbackWriteIt, &wd);
	ptWriter->finishWrite();

	return true;
}

// y up vs z up
// In COLMAP, the coordinate system is defined such that the Y-axis points downwards and the Z-axis points forward 
// In OpenMVS, the coordinate system is Z-up
// photogrammetry is Z-up
bool ExteriorsToColmap(MvsToSLPK_Options& opt, std::string &outFolder)
{
	std::string sparseFolder = outFolder;
	StdUtility::createFullDirectoryPath(sparseFolder);

	double boxData[9];
	readBoxFile(opt.boxFilename, boxData);
	
	std::string imagesTxt = sparseFolder;
	StdUtility::appendSlash(imagesTxt);
	imagesTxt += "images.txt";

	std::ofstream outFile;
	outFile.open(imagesTxt);
	if (!outFile.is_open()) {
		return false;
	}

	// This worked
	// Made work by comparing result to Geo Aligned
	int count = 1;
	for (auto& eoDat : opt.eoInfos) {
		Eigen::Matrix3d mOrg;
		mOrg = Eigen::AngleAxisd(eoDat.eoInfo.omega, Eigen::Vector3d::UnitX())
			* Eigen::AngleAxisd(eoDat.eoInfo.phi, Eigen::Vector3d::UnitY())
			* Eigen::AngleAxisd(eoDat.eoInfo.kappa, Eigen::Vector3d::UnitZ());
		

		Eigen::Matrix3d cbb{
								{1.0, 0.0, 0.0},
								{0.0, -1.0, 0.0},
								{0.0, 0.0, -1.0},
				};
		Eigen::Matrix3d m = mOrg * cbb;

		Eigen::Quaterniond q(m);

// 		o quickly aim a quaternion in the opposite direction, you can negate its vector part.This effectively rotates the quaternion by 180 degrees around its axis.Here's how you can do it:
// 
// 			Method 1: Negate the Vector Part
// 			If you have a quaternion(q = (w, x, y, z)), the opposite direction quaternion(q' ) is:
// //			[q' = (w, -x, -y, -z) ]
// 
		// our quaternion we calculated is exactly opposite of what we want.

		double px = eoDat.eoInfo.projCenterX - boxData[0];
		double py = eoDat.eoInfo.projCenterY - boxData[1];
		double pz = eoDat.eoInfo.projCenterZ - boxData[2];


		q.coeffs()[0] = -q.coeffs()[0];
		q.coeffs()[1] = -q.coeffs()[1];
		q.coeffs()[2] = -q.coeffs()[2];


		Eigen::Vector3d t2(px, py, pz);
		t2 = q * t2;

		outFile << std::fixed << std::setprecision(19)
			<< count << " " << q.w() << " " << q.x() << " " << q.y() << " " << q.z()
			//<< count << " " << q.w() << " " << -q.x() << " " << -q.y() << " " << -q.z()
			<< " " << -t2[0] << " " << -t2[1] << " " << -t2[2]
			<< " " << eoDat.eoInfo.cameraIndex + 1
			<< " " << eoDat.imageName
			<< std::endl;

		// add blank line
		outFile << std::endl;

		++count;
	}

	outFile.close();

	// create and write a camera file...
	std::string camerasTxt = sparseFolder;
	StdUtility::appendSlash(camerasTxt);
	camerasTxt += "cameras.txt";

	outFile.open(camerasTxt);
	if (!outFile.is_open()) {
		return false;
	}

	outFile << "# Camera list with one line of data per camera:" << std::endl;
	outFile << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]" << std::endl;
	outFile << "# Number of cameras: " << opt.camList.size() << std::endl;

	int camCount = 1;
	for (auto& camData : opt.camList) {

		if (camData.camParams.hasDistortion) {
			iSMTPRJ::CameraParameters& cp = camData.camParams;
			std::string camStr = StdUtility::string_format("%.2f, %.2f, %.2f, %.2f, %g, %g, %g, %g, %g, %g, %g, %g",
				cp.focal, cp.focal, cp.ppX, cp.ppY, cp.k[1], cp.k[2], cp.p[0], cp.p[1], cp.k[3], cp.k[4], cp.k[5], cp.k[6]);
			outFile
				<< camCount << " "
				<< "FULL_OPENCV "
				<< int(camData.camParams.filmWidth) << " " << int(camData.camParams.filmHeight) << " "
				<< camStr.c_str()
				<< std::endl;
		}
		else {
			outFile
				<< camCount << " "
				<< "SIMPLE_RADIAL "
				<< int(camData.camParams.filmWidth) << " " << int(camData.camParams.filmHeight) << " "
				<< std::fixed << std::setprecision(1)
				<< camData.camParams.focal << " "
				<< camData.camParams.ppX << " " << camData.camParams.ppY << " "
				<< std::fixed << std::setprecision(19)
				<< 0.0000000000000
				<< std::endl;
		}
		

		++camCount;
	}

	outFile.close();

	std::string points3DTxt = sparseFolder;
	StdUtility::appendSlash(points3DTxt);
	points3DTxt += "points3D.txt";

	outFile.open(points3DTxt);
	if (!outFile.is_open()) {
		return false;
	}

	outFile.close();

	return true;
}

static bool readDB(const std::string& dbFile, std::vector<std::string>& imageOrder)
{
	sqlite3* db;
	int rc = sqlite3_open(dbFile.c_str(), &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		std::cout << "Can't open database: " << sqlite3_errmsg(db);
		sqlite3_close(db);
		return false;
	}

	std::string tableName = "images";

	// 	char* zSql = 0;
	// 	zSql = sqlite3_mprintf("PRAGMA table_info=%Q", "images");

	std::string selectString = "SELECT * FROM ";
	selectString += tableName;
	selectString += " ORDER BY image_id";

	sqlite3_stmt* ppStmt = 0;
	rc = sqlite3_prepare_v2(db, selectString.c_str(), -1, &ppStmt, 0);
	if (rc != SQLITE_OK) {
		return false;
	}

	// each row has image_id, name, and camera_id
	std::vector<std::vector<std::string>> imageData;
	while (sqlite3_step(ppStmt) == SQLITE_ROW) {
		std::vector<std::string> newRow;
		int numCol = sqlite3_column_count(ppStmt);

		for (int i = 0; i < numCol; ++i) {
			int colType = sqlite3_column_type(ppStmt, i);
			const char* colText = (const char*)sqlite3_column_text(ppStmt, i);

			if (colText) {
				newRow.push_back(colText);
			}
			else {
				newRow.push_back(std::string());
			}
		}
		imageData.push_back(newRow);
	}

	rc = sqlite3_finalize(ppStmt);

	sqlite3_close(db);

	// data will be in the form imageID, imageName, cameraID
	// we just return the list of images
	for (auto& strList : imageData) {
		if (strList.size() >= 2) {
			imageOrder.push_back(strList[1]);
		}
	}

	return true;
}

// reorder our exterior by the image order in the sqlite database
bool rearrageImageOrderDB(MvsToSLPK_Options& opt, std::string& dbFile)
{
	std::vector<std::string> imageOrder;
	readDB(dbFile, imageOrder);

	// rearrange to order...
	std::vector<ExteriorInfoType> eoInfosNEW;
	for (auto& imageName : imageOrder) {
		for (auto& eoInfo : opt.eoInfos) {
			if (imageName == eoInfo.imageName) {
				eoInfosNEW.push_back(eoInfo);
			}
		}
	}

	opt.eoInfos = eoInfosNEW;

	return true;
}

bool imageTo8bit(const std::string& inputImageName, const std::string& outImage)
{
	auto iif = std::shared_ptr<IImageReader>(createIImageReader(), [](IImageReader* p) { p->Release(); });


	IImageReader& fm = *iif;
 	fm.ignoreUpperBands(true);

	if (!fm.setFile(inputImageName.c_str())) {
		return false;
	}

	GDALImageInfo info;
	fm.getImageInfo(info);

	bool hasGeoInfo = info._hasGeoInfo;
	IImageFormat_GeoInfo geoInfo;
 	if (hasGeoInfo)
 		fm.getGeoInfo(geoInfo);

	uint32_t imageWidth = info._imageWidth;
	uint32_t imageHeight = info._imageHeight;

	uint32_t tilesWidth = 256;
	uint32_t tilesHeight = 256;

	uint32_t tilesAcross = (imageWidth + tilesWidth - 1) / tilesWidth;
	uint32_t tilesDown = (imageHeight + tilesHeight - 1) / tilesHeight;
	uint32_t tilesPerImage = tilesAcross * tilesDown;

	auto iiWrite = std::shared_ptr<IImageWriter>(createIImageWriter(), [](IImageWriter* p) { p->Release(); });

	IImageWriter& imageWriter = *iiWrite;
	GDALWriteOptions opt;
	opt.bytesPerChannel = 1;
	opt.tileWidth = tilesWidth;
	opt.tileHeight = tilesHeight;
	opt.width = imageWidth;
	opt.height = imageHeight;
	opt.numChannels = 3;
	opt.hasGeoInfo = hasGeoInfo;
 	if (hasGeoInfo)
 		opt.geoInfo = geoInfo;
	opt.generateZoomLevels = true;

	imageWriter.start(outImage.c_str(), opt);

	for (uint32_t i = 0; i < tilesPerImage; ++i) {
		uint32_t row = i / tilesAcross;
		uint32_t col = i - (row * tilesAcross);

		int left = col * tilesWidth;
		int top = row * tilesHeight;
		int right = left + tilesWidth;
		int bottom = top + tilesHeight;

#pragma warning(suppress: 4018)
		if (right > imageWidth) {
			right = imageWidth;
		}
#pragma warning(suppress: 4018)
		if (bottom > imageHeight) {
			bottom = imageHeight;
		}

		uint32_t actualWidth = right - left;
		uint32_t actualHeight = bottom - top;

		uint8_t* dataPixel = new uint8_t[fm.readRectMemSize(actualWidth, actualHeight)];
		fm.readRect(left, top, actualWidth, actualHeight, dataPixel);

		if (info._bytesPerChannel == 2) {
			// convert...
			int dataSize = actualWidth * actualHeight * info._numChannels;
			uint8_t* newDataPixel = new uint8_t[dataSize];
			WORD* wordPtr = (WORD*)dataPixel;
			for (int i = 0; i < dataSize; ++i) {
				newDataPixel[i] = static_cast<uint8_t>((wordPtr[i] / 4095.0) * 255);
			}

			delete[] dataPixel;
			dataPixel = newDataPixel;
		}

		//memset(imageTile, 255, actualWidth * actualHeight * 3);

		//write and generate new image
		imageWriter.writeTile((char*)dataPixel);

		delete[] dataPixel;

		//std::cout << i << " " << tilesPerImage << std::endl;
	}

	imageWriter.finish();

	return true;
}

void postProgress(int progress, const char* msg)
{
	static bool inited = false;
	static HANDLE hPipe = nullptr;

	if (!inited) {
		hPipe = CreateFile(
			TEXT("\\\\.\\pipe\\MVStoSLPK"),
			GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			return;
		}

		inited = true;
	}


	std::string messageToSend;
	if (msg) {
		messageToSend = StdUtility::string_format("%d|%s", progress, msg);
	}
	else {
		messageToSend = StdUtility::string_format("%d", progress);
	}
	

	DWORD bytesWritten;
	BOOL success = WriteFile(hPipe, messageToSend.c_str(), strlen(messageToSend.c_str()), &bytesWritten, NULL);
	
// 	if (success)
// 	{
// 		std::cout << "Message sent: " << messageToSend.c_str() << std::endl;
// 	}


	if (progress == 100) {
		// clean up at 100 percent
		CloseHandle(hPipe);
		inited = false;
	}
}


bool readParamFile(MvsToSLPK_Options& opt, CmdLineParams &cp)
{
	std::string paramFilename = opt.workingFolder;
	StdUtility::appendSlash(paramFilename);
	paramFilename += "param.txt";

	std::ifstream	file;
	file.open(paramFilename);

	if (!file.is_open()) {
		return false;
	}
	const int BUFFER_SIZE = 4000;
	char	buff[BUFFER_SIZE + 1] = "";

	int lineNumber = 0;

	std::vector<std::string> lines;
	while (file.getline(buff, BUFFER_SIZE).good()) {
		std::string lineData = buff;
		lines.push_back(lineData);
	}
	file.close();


	if (lines.size() >= 5) {
		cp.maxImageSize = atoi(lines[0].c_str());

		cp.smtProjectFile = lines[1];
		cp.workingFolder = lines[2];
		cp.resultFolder = lines[3];
		cp.projectionFile = lines[4];

		return true;
	}

	return false;
}

void printPLYresults(MvsToSLPK_Options& opt)
{
	std::cout << std::endl;

	for (auto& plyFile : opt.texturePlyResults) {

		try {
			happly::PLYData plyIn(StdUtility::convert(plyFile));

			std::cout << "PLY file: " << StdUtility::convert(plyFile) << std::endl;

			size_t vertexCount = plyIn.getElement("vertex").getProperty<float>("x").size();

			//auto &faceElement = plyIn.getElement("face");
			size_t faceCount = plyIn.getElement("face").getListProperty<uint32_t>("vertex_indices").size();

			std::cout << "Vertex Count: " << vertexCount << " Face Count: " << faceCount << std::endl;
		}
		catch (...) {
			std::cout << "PLY file: " << StdUtility::convert(plyFile) << " Failed to open!" << std::endl;
		}
	}

	std::cout << std::endl;
}