#pragma once

#include "Eigen/Geometry"
#include "StdUtil/StdUtility.h"

#define iSMTPRJ ISmtPrj
#include "SmtPrj/SmtPrj.h"

DWORD execute(const std::wstring& executable, const std::wstring& cmdline);
DWORD executeColmap(const std::wstring& executable, const std::wstring& cmdline);


class MvsToSLPK_EXES
{
public:
#if 1
	// exe COLMAP
	std::wstring colmap_EXE = L"colmap.exe";
	// exe OpenMVS
	std::wstring InterfaceCOLMAP_EXE = L"InterfaceCOLMAP.exe";
	std::wstring DensifyPointCloud_EXE = L"DensifyPointCloud.exe";
	std::wstring ReconstructMesh_EXE = L"ReconstructMesh.exe";
	std::wstring RefineMesh_EXE = L"RefineMesh.exe";
	std::wstring textureMesh_EXE = L"textureMesh.exe";

	void setToDeveloperLocations()
	{
		colmap_EXE = L"J:\\COLMAP\\bin\\colmap.exe";
		// exe OpenMVS
		InterfaceCOLMAP_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\InterfaceCOLMAP.exe";
		DensifyPointCloud_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\DensifyPointCloud.exe";
		ReconstructMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\ReconstructMesh.exe";
		RefineMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\RefineMesh.exe";
		textureMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\textureMesh.exe";
	}

	bool setToPath(std::wstring &folder)
	{
		std::wstring dir = folder;
		StdUtility::appendSlash(dir);

		std::wstring colMapDir = dir;
		colMapDir += L"COLMAP\\bin";
		StdUtility::appendSlash(colMapDir);

		std::wstring mvsDir = dir;
		mvsDir += L"openMVS";
		StdUtility::appendSlash(mvsDir);


		colmap_EXE = colMapDir;
		// exe OpenMVS
		InterfaceCOLMAP_EXE = mvsDir;
		DensifyPointCloud_EXE = mvsDir;
		ReconstructMesh_EXE = mvsDir;
		RefineMesh_EXE = mvsDir;
		textureMesh_EXE = mvsDir;

		colmap_EXE += L"colmap.exe";
		// exe OpenMVS
		InterfaceCOLMAP_EXE += L"InterfaceCOLMAP.exe";
		DensifyPointCloud_EXE += L"DensifyPointCloud.exe";
		ReconstructMesh_EXE += L"ReconstructMesh.exe";
		RefineMesh_EXE += L"RefineMesh.exe";
		textureMesh_EXE += L"textureMesh.exe";

		return StdUtility::fileExists(colmap_EXE);
	}
#else
	// exe COLMAP
	std::wstring colmap_EXE = L"J:\\COLMAP\\bin\\colmap.exe";
	// exe OpenMVS
	std::wstring InterfaceCOLMAP_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\InterfaceCOLMAP.exe";
	std::wstring DensifyPointCloud_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\DensifyPointCloud.exe";
	std::wstring ReconstructMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\ReconstructMesh.exe";
	std::wstring RefineMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\RefineMesh.exe";
	std::wstring textureMesh_EXE = L"J:\\Dev3\\openMVS\\Buildv17\\bin\\vc17\\x64\\Release\\textureMesh.exe";
#endif
};



struct ExteriorInfoType
{
	iSMTPRJ::EOImageInfo eoInfo;
	std::string imageName;
};
struct CameraDataType
{
	iSMTPRJ::CameraParameters camParams;

	double scale = 1.0;

	iSMTPRJ::CameraParameters mmCamParams;
};

class MvsToSLPK_Options
{
public:
	// inputs
	std::string workingFolder; // all processing done here
	std::string resultsFolder; // the final results

	std::string summitProject;
	std::string prjFile;

	// SPLK options
	bool generate3DTiles = true;
	bool generateSLPK = true;
	int splitDivisions = 1; // 0 means no split (1 box), 1 means 2x2 (4 boxes), 2 - means 4x4 (16 boxes), 3 - means 8x8 (64 boxes) etc.

	// should always be true
	bool refineMesh = true;

	bool hasCuda = false;

	bool alwaysUseKnownCameraPose = false;

	// if you set --max_image_size 2000, COLMAP will resize the images so that neither the width nor the height exceeds 2000 pixels.This can be useful for managing memory usage and processing time, especially when working with high resolution images
	// https ://github.com/colmap/colmap/issues/2361 https://github.com/mwtarnowski/colmap-parameters
	int distortMaxImageSize = 6000;

	enum class GenerationType {SPARSE, DENSE, BOTH};
	GenerationType gType = GenerationType::BOTH;

	bool allowMVS_split = false; // allow MVS to split the scene into smaller sub-scenes

	//
	// outputs
	//
	unsigned int cpu_cores = 1;

	std::string imagesFolder; // obtained from summit project
	std::vector<std::string> imagesToProcess;
	std::string imageListTxt;

	// for metric or fixed camers...like from a Summit Project
	std::vector<CameraDataType> camList;

	bool hasCamera = false;
	bool fixedCameraParameters = false;
	int imageWidth = 0;
	int imageHeight = 0;

	std::string boxFilename;
	std::vector<ExteriorInfoType> eoInfos;

	bool hasCoordSystem = true;

	std::vector<std::wstring> texturePlyResults;
	std::vector<std::wstring> densePtsPlyResults;

	// splitFiles
	std::vector<std::vector<std::vector<std::string>>> vectorOfSplitFiles;

	// other
	MvsToSLPK_EXES exes;
};

bool PlyFileToLAS(std::vector<std::wstring> &densePtsPlyResults, const std::string& outLasFile, const std::string& boxFilename);

bool PlyFileToLAS(const std::string& firstFile, const std::string& secondFile, const std::string& boxFilename);

bool resizeTexturePow2(const std::string& firstFile);

bool writeBoxFile(const std::string& boxFilename, const Eigen::Vector3d& offsetPt, const Eigen::Vector3d& bbMin, const Eigen::Vector3d& bbMax);
bool colmapPtsToLas(const std::string& boxFilename, const std::string &ptFilename, const std::string& lasFilename);


bool ExteriorsToColmap(MvsToSLPK_Options &opt, std::string& outFolder);
bool rearrageImageOrderDB(MvsToSLPK_Options& opt, std::string& dbFile);


bool imageTo8bit(const std::string& inputImageName, const std::string& outImage);
void postProgress(int progress, const char *msg = nullptr);

struct CmdLineParams
{
	int maxImageSize = 1000;
	std::string smtProjectFile;
	std::string workingFolder;
	std::string resultFolder;
	std::string projectionFile;
};
bool readParamFile(MvsToSLPK_Options& opt, CmdLineParams& cp);

void printPLYresults(MvsToSLPK_Options& opt);