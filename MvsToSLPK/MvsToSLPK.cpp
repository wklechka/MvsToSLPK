// MvsToSLPK.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>

#include <tclap/CmdLine.h>

#include "StdUtil/StdUtility.h"
#include "StdUtil/WinUtility.h"
#include "StdUtil/MathFunc.h"

#include "Eigen/Geometry"

#include "MvsCommon.h"
#include "TeeBuf.h"

#include "SmtPrj/SmtPrj.h"

#include "slpk3DTile/slpk3DTiles.h"

#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ImageRW/IImageReader.h"
#include "ImageRW/IImageWriter.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// forward declarations
bool isColmapDone(MvsToSLPK_Options& opt);
bool writeGeoInfo(MvsToSLPK_Options& opt, std::shared_ptr<iSMTPRJ>& smtPrj);
bool runCOLMAP_fixedCameras(MvsToSLPK_Options& opt);
bool runCOLMAP2(MvsToSLPK_Options& opt);
bool runCOLMAP(MvsToSLPK_Options& opt, bool endAtSpase = false);
bool colmapToScene(MvsToSLPK_Options& opt);
bool runOpenMvsWSplit(MvsToSLPK_Options& opt, bool splitUp);
void makeSplitFiles(MvsToSLPK_Options& opt, std::vector<std::vector<std::string>>& lodSplitfiles);
bool generateSLPK(MvsToSLPK_Options& opt, std::vector<std::vector<std::string>>& lodSplitfiles, std::shared_ptr<iSMTPRJ>& smtPrj);

// MAIN FUNCTION
int main()
{
	std::cout << "MsvToSLPK:  Started" << std::endl;

	int t1 = clock();

	std::vector<std::string> argStdList;

	LPWSTR* szArglist;
	int nArgs;
	int i;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (NULL == szArglist)
	{
		wprintf(L"CommandLineToArgvW failed\n");
		return 0;
	}
	else {
		for (i = 0; i < nArgs; i++) {
			std::wstring tmpStr = szArglist[i];
			argStdList.push_back(StdUtility::convert(tmpStr));
		}
	}

	bool colmapAlreadyRan = false;
	enum class ProcessingType { FULL, COLMAP_ONLY, MVS_ONLY };
	ProcessingType processType = ProcessingType::FULL;

	MvsToSLPK_Options opt;

	try {
		TCLAP::CmdLine cmd("Command MvsToSLPK description message", ' ', "1.0");

		// Define a value argument and add it to the command line.
		// A value arg defines a flag and a type of value that it expects,
		// such as "-n Bishop".
		TCLAP::ValueArg<std::string> summitArg("s", "SummitProj", "Summit Project file", true, "", "string");
		TCLAP::ValueArg<std::string> workFolderArg("w", "workFolder", "Working folder", true, "", "string");
		TCLAP::ValueArg<std::string> resultsFolderArg("r", "resultsFolder", "Results folder", true, "", "string");
		TCLAP::ValueArg<std::string> prjArg("p", "ProjectionFile", "Projection File", false, "", "string");
		TCLAP::SwitchArg knownArg("k", "UseKnown", "Flag to use known camera poses. Or recalc if EXIF info", true);
		TCLAP::ValueArg<int> maxSizeArg("m", "MaxImageSize", "Max image size used by undistort", false, 6000, "Integer Value");
		TCLAP::ValueArg<int> generationTypeArg("g", "Generation", "1-sparse only, 2-dense only, 3-both", false, 3, "Integer Value");
		TCLAP::ValueArg<int> processingArg("j", "Process", "0-full processing, 1-COLMAP only, 2-OpenMVS only", false, 0, "Integer Value");
		TCLAP::ValueArg<int> allowMVS_split_Arg("a", "AllowMVSsplit", "0-No, 1-Yes", false, 0, "Integer Value");


		cmd.add(summitArg);
		cmd.add(workFolderArg);
		cmd.add(resultsFolderArg);
		cmd.add(prjArg);
		cmd.add(knownArg);
		cmd.add(maxSizeArg);
		cmd.add(generationTypeArg);
		cmd.add(processingArg);
		cmd.add(allowMVS_split_Arg);

		// Parse the argv array.
		cmd.parse(argStdList);

		opt.summitProject = summitArg.getValue();
		opt.workingFolder = workFolderArg.getValue();
		opt.resultsFolder = resultsFolderArg.getValue();
		opt.prjFile = prjArg.getValue();
		opt.alwaysUseKnownCameraPose = knownArg.getValue();
		opt.distortMaxImageSize = maxSizeArg.getValue();
		opt.allowMVS_split = allowMVS_split_Arg.getValue() == 1 ? true : false;

		switch (generationTypeArg.getValue())
		{
		case 1:
			opt.gType = MvsToSLPK_Options::GenerationType::SPARSE;
			break;
		case 2:
			opt.gType = MvsToSLPK_Options::GenerationType::DENSE;
			break;
		case 3:
			opt.gType = MvsToSLPK_Options::GenerationType::BOTH;
			break;
		default:
			opt.gType = MvsToSLPK_Options::GenerationType::BOTH;
			break;
		}

		switch (processingArg.getValue())
		{
		case 0:
			processType = ProcessingType::FULL;
			break;
		case 1:
			processType = ProcessingType::COLMAP_ONLY;
			break;
		case 2:
			processType = ProcessingType::MVS_ONLY;
			break;
		default:
			processType = ProcessingType::FULL;
			break;
		}

		colmapAlreadyRan = isColmapDone(opt);

		// make a param file
		std::string paramFilename = opt.workingFolder;
		StdUtility::appendSlash(paramFilename);
		paramFilename += "param.txt";
		std::ofstream outParam;
		outParam.open(paramFilename);
		if (outParam.is_open()) {
			outParam << maxSizeArg.getValue() << std::endl;
			outParam << summitArg.getValue() << std::endl;
			outParam << workFolderArg.getValue() << std::endl;
			outParam << resultsFolderArg.getValue() << std::endl;
			outParam << prjArg.getValue() << std::endl;
		}
		outParam.close();
	}
	catch (TCLAP::ArgException& e)  // catch exceptions
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	//postProgress(0);
	postProgress(1, "MsvToSLPK: Started");

	StdUtility::createFullDirectoryPath(opt.workingFolder);
	StdUtility::createFullDirectoryPath(opt.resultsFolder);

	// set up a LOG file
	std::string logFilename = opt.workingFolder;
	StdUtility::appendSlash(logFilename);
	logFilename += "logfile.txt";

	// send cout to terminal and file
	std::ofstream log_file(logFilename);
	TeeBuf teeBuf(std::cout.rdbuf(), log_file.rdbuf());
	std::streambuf* oldCoutBuf = std::cout.rdbuf(&teeBuf);

	auto file_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log_file);
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	spdlog::logger logger("multi_sink", { file_sink, console_sink });
	spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));


	for (i = 0; i < nArgs; i++) {
		std::string printStr = StdUtility::string_format("%d: %ws", i, szArglist[i]);
		std::cout << printStr.c_str() << std::endl;
	}

	// find the EXEs or default to programmer environment
	std::wstring thisEXEpath = WinUtility::executable_path();
	std::wstring exeDir = StdUtility::getDirectory(thisEXEpath);
	if (!opt.exes.setToPath(exeDir)) {
		opt.exes.setToDeveloperLocations();
	}

	spdlog::info("MvsToSLPK begin");

	opt.cpu_cores = std::thread::hardware_concurrency();
	std::cout << "Number of CPU cores: " << opt.cpu_cores << std::endl;

	// make sure folders exist
	if (!StdUtility::fileExists(opt.summitProject)) {
		std::cout << "Summit project not found: " << opt.summitProject.c_str() << std::endl;
		return -1;
	}

	// must have GDAL and PROJ data set in ProgramData
	std::wstring  programDataGDAL = WinUtility::common_APPDATA(L"MvsToSLPK\\GDAL");
	if (!StdUtility::fileExists(programDataGDAL)) {
		std::wcout << "GDAL folder not found: " << programDataGDAL.c_str() << std::endl;
		std::wcout << "GDAL data must be stored here" << std::endl;
		return -1;
	}
	else {
		std::wcout << "GDAL data found at: " << programDataGDAL.c_str() << std::endl;
	}

	// write geo information file for COLMAP also write a BOX file of the Geo offset

	// creates geo information (and box file) from Summit project 
	std::cout << "Starting: writeGeoInfo" << std::endl;

	auto smtPrj = std::shared_ptr<iSMTPRJ>(createISmtPrj(), [](iSMTPRJ* p) { p->Release(); });

	if (!writeGeoInfo(opt, smtPrj)) {
		// cant continue if this failed
		spdlog::error("writeGeoInfo reported false.");
		return -1;
	}

	if (processType == ProcessingType::FULL || processType == ProcessingType::COLMAP_ONLY) {
		postProgress(5, "Starting COLMAP");


		if (colmapAlreadyRan) {
			// COLMAP has already been run
			std::cout << "COLMAP previously ran, SKIPPING this step." << std::endl;
		}
		else
		{
			// COLMAP will make the sparse model and finish with undistorted dense folder

			if (false && opt.fixedCameraParameters) {
				//if (opt.fixedCameraParameters) {
					// This is not working
					// point_triangulator keeps modifying the camera
					// we will go ahead and use mapper - even though it does another bundle adjustment
					// and takes longer but the result we way better with the distortion in the camera
				if (!runCOLMAP_fixedCameras(opt)) {
					return -1;
				}
			}
			else {
				// COLMAP will make the sparse model and finish with undistorted dense folder
				std::cout << "Starting: colmap" << std::endl;
				if (!runCOLMAP(opt)) {
					// cant continue if this failed
					spdlog::error("runCOLMAP reported false.");
					return -1;
				}
			}

			colmapToScene(opt);
		}
	}

	if (processType == ProcessingType::COLMAP_ONLY) {
		postProgress(20, "COLMAP: Finished");
		return 0;
	}

	if (processType == ProcessingType::FULL || processType == ProcessingType::MVS_ONLY) {

		postProgress(20, "OpenMVS: Started");

		if (opt.gType == MvsToSLPK_Options::GenerationType::SPARSE)
		{
			std::cout << "Sparse only selected.  Finished." << std::endl;
			postProgress(100);
			return 0;
		}

#if 1
		// converts undistorted folder to MVS model and converts it dense
		// dense is further processed to Mesh and finally a Textured Mesh.
		// 
	// 	std::cout << "Starting: MVS" << std::endl;
	// 	if (!runOpenMvs(opt)) {
	// 		// cant continue if this failed
	// 		spdlog::error("runOpenMvs reported false.");
	// 		return -1;
	// 	}

		std::cout << std::endl << "Starting: MVS - Multi" << std::endl;
		if (!runOpenMvsWSplit(opt, opt.allowMVS_split)) {
			//if (!runOpenMvsWSplit(opt, true)) {
				// cant continue if this failed
			spdlog::error("runOpenMvs reported false.");
			return -1;
		}
#endif
	}

	// print out some information about the generated PLY...
	printPLYresults(opt);

	if (processType == ProcessingType::MVS_ONLY) {
		postProgress(20, "OpenMVS: Finished");
		return 0;
	}

	postProgress(80, "Make Split files");

	if (opt.hasCoordSystem) {
		// 3D Tiles and SLPK generation
		std::vector<std::vector<std::string>> lodSplitfiles;
		std::cout << "Starting: Split Files" << std::endl;
		makeSplitFiles(opt, lodSplitfiles);

		postProgress(85, "SLPK and 3D Tile generation");
		std::cout << "Starting: SLPK and 3D Tile generation" << std::endl;
		generateSLPK(opt, lodSplitfiles, smtPrj);
	}

	postProgress(95);


	int t2 = clock();

	std::cout << "Total time: " << float(t2 - t1) / CLOCKS_PER_SEC << " seconds" << std::endl;

	spdlog::info("MvsToSLPK end");

	postProgress(100);

	// Restore the original stream buffer
	std::cout.rdbuf(oldCoutBuf);

	return 0;
}


// re writes the camera file
// Undistort function does not properly change the camera if there was no distortion
// but undistort is always called because it not only removes distortion but also changes the image size
// its easy to just write out the proper camera that should have been made...
// 
static bool overwriteDenseCam_UndistortFIX(MvsToSLPK_Options& opt)
{
	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring imageListTxt = StdUtility::convert(opt.imageListTxt);

	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";
	std::wstring sparseFolder = denseFolder;
	sparseFolder += L"\\sparse";

	// overwrite the camera file

	// create and write a camera file...
	std::string camerasTxt = StdUtility::convert(sparseFolder);
	StdUtility::appendSlash(camerasTxt);
	camerasTxt += "cameras.txt";

	std::ofstream outFile;
	outFile.open(camerasTxt);
	if (!outFile.is_open()) {
		return false;
	}

	//1 PINHOLE 7666 13800 10000 10000 3833 6900
	outFile << "# Camera list with one line of data per camera:" << std::endl;
	outFile << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]" << std::endl;
	outFile << "# Number of cameras: " << opt.camList.size() << std::endl;

	double scale = 1.0;
	if (opt.imageHeight >= opt.distortMaxImageSize || opt.imageWidth >= opt.distortMaxImageSize) {
		if (opt.imageWidth > opt.imageHeight) {
			scale = double(opt.distortMaxImageSize) / double(opt.imageWidth);
		}
		else {
			scale = double(opt.distortMaxImageSize) / double(opt.imageHeight);
		}
	}

	for (int camIndex = 0; camIndex < opt.camList.size(); ++camIndex) {
		double focal = double(opt.camList[camIndex].camParams.focal) * scale;
		double ppX = double(opt.camList[camIndex].camParams.ppX) * scale;
		double ppY = double(opt.camList[camIndex].camParams.ppY) * scale;
		int w = opt.imageWidth;
		int h = opt.imageHeight;

		if (scale != 1.0) {
			if (opt.imageWidth < opt.imageHeight) {
				w = MathFunc2::iround(double(opt.imageWidth) * scale);
				h = opt.distortMaxImageSize;
			}
			else {
				h = MathFunc2::iround(double(opt.imageHeight) * scale);
				w = opt.distortMaxImageSize;
			}
		}

		outFile << camIndex + 1 << " PINHOLE "
			<< w << " " << h << " "
			<< std::fixed << std::setprecision(2)
			<< focal << " "
			<< focal << " "
			<< ppX << " " << ppY
			<< std::endl;
	}

	outFile.close();

	return true;
}

static void preprocessCameraAndCoordSys(MvsToSLPK_Options& opt, std::shared_ptr<iSMTPRJ>& smtPrj)
{
	// fetch the camera parameters
	if (smtPrj->numCameras() > 0) {

		iSMTPRJ::CameraParameters camParams;
		if (smtPrj->getCameraInfo(0, camParams)) {

			// convert to pixel
			iSMTPRJ::EOImageInfo eoInfo;
			if (smtPrj->getEOInfo(0, eoInfo)) {

				std::string imageFile = smtPrj->getImage(0);

				if (opt.prjFile.empty()) {
					// not projection file...

					opt.hasCoordSystem = false;
				}

				if (eoInfo.imageWidth > 0 && eoInfo.imageHeight > 0) {

					opt.imageWidth = eoInfo.imageWidth;
					opt.imageHeight = eoInfo.imageHeight;

					opt.hasCamera = true;
				}
				else {
					// if the project did not have imageWidth and Height
					// get it from the image
				}
			}
		}
	}

	// ignore having EXIF and don't recalc poses
	// Normal summit project would never recalc
	//if (opt.alwaysUseKnownCameraPose) {
		opt.fixedCameraParameters = true;
		opt.hasCamera = true;
	//}
}

// write geo information file for COLMAP also write a BOX file of the Geo offset
static bool writeGeoInfo(MvsToSLPK_Options& opt, std::shared_ptr<iSMTPRJ> &smtPrj)
{
	std::string summitProject = opt.summitProject;
	std::string workfolder = opt.workingFolder;

	if (summitProject.empty() || workfolder.empty()) {
		return false;
	}

	// need a summit project and scene.mvs to create the transform
	if (!smtPrj->setFile(summitProject.c_str())) {
		return false;
	}

	// TEST write smtfpj
// 	std::string outTestName = StdUtility::appendName(summitProject, "TESTWrite");
// 	smtPrj->writeProject(outTestName.c_str());

	std::vector<Eigen::Vector3d> projectionCenters;
	int numImages = smtPrj->numImages();

	if (numImages < 1)
		return false;

	preprocessCameraAndCoordSys(opt, smtPrj);

	for (int i = 0; i < numImages; ++i) {

		std::string imageStr = smtPrj->getImage(i);

		opt.imagesToProcess.push_back(imageStr);

		iSMTPRJ::EOImageInfo eoInfo;
		smtPrj->getEOInfo(i, eoInfo);

		ExteriorInfoType eoDat;
		eoDat.eoInfo = eoInfo;
		eoDat.imageName = StdUtility::getName(imageStr, false);
		opt.eoInfos.push_back(eoDat);

		Eigen::Vector3d newVect;
		newVect[0] = eoInfo.projCenterX;
		newVect[1] = eoInfo.projCenterY;
		newVect[2] = eoInfo.projCenterZ;
		projectionCenters.push_back(newVect);
	}

	if (opt.imagesToProcess.size() == 0) {
		// no images in project
		return false;
	}

	bool mustConvertImages = false;
	for (auto& imagePath : opt.imagesToProcess) {
		// check if any are: not 8 bit, not 3 band...will make all image 8 bit and 3 channel
		auto iif = std::shared_ptr<IImageReader>(createIImageReader(), [](IImageReader* p) { p->Release(); });

		IImageReader& fm = *iif;

		if (fm.setFile(imagePath.c_str())) {

			GDALImageInfo info;
			if (!fm.getImageInfo(info)) {
				return false;
			}

			if (info._bytesPerChannel != 1 || info._numChannels > 3 || info._numChannels == 1) {
				mustConvertImages = true;
				break;
			}
		}
	}

	// check that all images are in the same folder - COLMAP seems to require this
	if (!mustConvertImages) {
		std::string checkFolder = StdUtility::getDirectory(opt.imagesToProcess[0]);

		for (auto& imagePath : opt.imagesToProcess) {
			std::string folder = StdUtility::getDirectory(imagePath);
			if (StdUtility::CompareNoCase(folder, checkFolder) != 0) {
				mustConvertImages = true;
				break;
			}
		}
	}

	// test!!!
	//mustConvertImages = true;

	if (mustConvertImages) {
		// make an 8bit image folder
		std::string image8bitFolder = workfolder;
		StdUtility::appendSlash(image8bitFolder);
		image8bitFolder += "Image8Bit";
		StdUtility::createFullDirectoryPath(image8bitFolder);

		std::vector<std::string> newImagesToProcess;

		std::cout << "Converting images to 8 bit, 3 band." << std::endl;
		for (auto& imagePath : opt.imagesToProcess) {
			std::string imageName = StdUtility::getName(imagePath);
			std::string outImage = image8bitFolder;
			StdUtility::appendSlash(outImage);
			outImage += imageName;
			outImage += ".tif";
			imageTo8bit(imagePath, outImage);

			newImagesToProcess.push_back(outImage);
		}

		opt.imagesToProcess = newImagesToProcess;

		// correct the eoInfos
		int index = 0; 
		for (auto& imagePath : opt.imagesToProcess) {
			ExteriorInfoType &eoDat = opt.eoInfos[index];
			eoDat.imageName = StdUtility::getName(imagePath, false);
			++index;
		}
	}

	// make the camera list for fixed parameter cameras
	int numCameras = smtPrj->numCameras();
	for (int camIndex = 0; camIndex < numCameras; ++camIndex) {
		iSMTPRJ::CameraParameters camParams;
		if (smtPrj->getCameraInfo(camIndex, camParams)) {

			// default w, h
			int w = opt.imageWidth;
			int h = opt.imageHeight;

			// find an EO and use its height width...this may be remove if we can assume
			// all images have the same h and w
			for (auto& eo : opt.eoInfos) {
				if (eo.eoInfo.cameraIndex == camIndex) {
					w = eo.eoInfo.imageWidth;
					h = eo.eoInfo.imageHeight;
					break;
				}
			}

			double pixScale = camParams.filmWidth / double(w);

			CameraDataType camData;
			camData.mmCamParams = camParams;

			iSMTPRJ::CameraParameters camPixel = camParams;

			camPixel.focal = camParams.focal / pixScale;
			camPixel.filmWidth = w;
			camPixel.filmHeight = h;
			camPixel.ppX = camParams.ppX / pixScale + double(w) / 2.0;
			camPixel.ppY = camParams.ppY / pixScale + double(h) / 2.0;

			camData.camParams = camPixel;
			camData.scale = pixScale;

			opt.camList.push_back(camData);
		}
	}

	

	// set the images folder...for now assume all in same directory
	// they have to be for colmap to work!
	opt.imagesFolder = StdUtility::getDirectory(opt.imagesToProcess[0], true);


	// make image_list.txt
	std::string imageListFilename = workfolder;
	StdUtility::appendSlash(imageListFilename);
	imageListFilename += "image_list.txt";
	std::ofstream imageList_outFile;
	imageList_outFile.open(imageListFilename);
	if (!imageList_outFile.is_open()) {
		return false;
	}
	for (auto &imageStr : opt.imagesToProcess) {
		std::string imageName = StdUtility::getName(imageStr, false);
		imageList_outFile << imageName.c_str() << std::endl;
	}
	imageList_outFile.close();
	opt.imageListTxt = imageListFilename;

	// TODO make and offset first

	Eigen::Vector3d centroid_source = Eigen::Vector3d::Zero();
	for (size_t i = 0; i < projectionCenters.size(); ++i) {
		centroid_source += projectionCenters[i];
	}
	centroid_source /= double(projectionCenters.size());

	// do min max
	Eigen::Vector3d bbMin = centroid_source;
	Eigen::Vector3d bbMax = centroid_source;
	for (size_t i = 0; i < projectionCenters.size(); ++i) {
		for (int j = 0; j < 3; ++j) {
			bbMin[j] = std::min(bbMin[j], projectionCenters[i][j]);
			bbMax[j] = std::max(bbMax[j], projectionCenters[i][j]);
		}
	}

	// offset the coord sys
	// It is very important that the offset be at the center of X, Y and Z.
	// Z is the more tricky part since the project center Z is nowhere near the actual Zs of point cloud
	// Z set to 0.0 works ok, 
	// setting to centroid Z of projection centers does not work well for OpenMVS the oobs is messed and set to centroid not what we set as projection centers
	// really need the centroid of the actual Z of points as the offset
	// Now using the Average Z for the project...thought about re-calcing Geo.TXT and boxfile form Point3d.txt...still might then we could get the real X, Y, Z centers of the sparse points
	centroid_source[2] = smtPrj->averageGroundZ();

	for (size_t i = 0; i < projectionCenters.size(); ++i) {
		projectionCenters[i] -= centroid_source;
	}
	// end write geo file

	std::string boxFilename = workfolder;
	StdUtility::appendSlash(boxFilename);
	boxFilename += "geo.box";
	writeBoxFile(boxFilename, centroid_source, bbMin, bbMax);

	opt.boxFilename = boxFilename;

	// write geo.txt...offset projection centers for each image
	// only used by colmap model_aligner

	std::string geoFilename = workfolder;
	StdUtility::appendSlash(geoFilename);
	geoFilename += "geo.txt";

	// write geo file
	std::ofstream outFile;
	outFile.open(geoFilename);
	if (!outFile.is_open()) {
		return false;
	}
	for (int i = 0; i < opt.imagesToProcess.size(); ++i) {
		std::string name = StdUtility::getName(opt.imagesToProcess[i], false);
		auto& pc = projectionCenters[i];
		outFile << name << " " << pc[0] << " " << pc[1] << " " << pc[2] << std::endl;
	}
	outFile.close();

	return true;
}

static void writeSparsePts(MvsToSLPK_Options& opt)
{
	std::string sparseFolder = opt.workingFolder;
	sparseFolder += "\\sparse";
	StdUtility::createFullDirectoryPath(sparseFolder);

	// Model aligner - adds georeferencing to the model
	std::string geoFolder = sparseFolder;
	geoFolder += "\\GeoModel";
	StdUtility::createFullDirectoryPath(geoFolder);

	// sparse points to LAS
	// ply to las...
	std::string boxFilename = opt.workingFolder;
	StdUtility::appendSlash(boxFilename);
	boxFilename += "geo.box";

	std::string pointCloudFileResult = opt.resultsFolder;
	StdUtility::appendSlash(pointCloudFileResult);
	pointCloudFileResult += "SparsePointCloud.las"; // PLY not geo reference so useless...convert to LAS

	std::string ptFile = geoFolder;
	StdUtility::appendSlash(ptFile);
	ptFile += "points3D.txt";

	if (opt.gType != MvsToSLPK_Options::GenerationType::DENSE)
		colmapPtsToLas(boxFilename, ptFile, pointCloudFileResult);
}

// Generate MVS from COLMAP project
// makes the .mvs file
static bool colmapToScene(MvsToSLPK_Options& opt)
{
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";

	std::wstring cmdline = StdUtility::string_format(L"-i \"%s\" -o \"%s\\scene.mvs\" -w \"%s\" --image-folder \"%s\\images\"", denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.InterfaceCOLMAP_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	execute(opt.exes.InterfaceCOLMAP_EXE, cmdline);

	postProgress(25);

	return true;
}

// can be used to test if COLMAP had been previously run
static bool isColmapDone(MvsToSLPK_Options& opt)
{
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense\\scene.mvs";

	CmdLineParams cp;
	if (!readParamFile(opt, cp)) {
		return false;
	}

	if (cp.maxImageSize != opt.distortMaxImageSize) {
		return false;
	}

	return StdUtility::fileExists(denseFolder);
}

static bool runCOLMAP2(MvsToSLPK_Options& opt)
{
	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring imageListTxt = StdUtility::convert(opt.imageListTxt);

	std::wstring sparseFolder = workingFolder;
	sparseFolder += L"\\sparse";
	StdUtility::createFullDirectoryPath(sparseFolder);

	// Model aligner - adds georeferencing to the model
	std::wstring geoFolder = sparseFolder;
	geoFolder += L"\\GeoModel";
	StdUtility::createFullDirectoryPath(geoFolder);
	
	// NOTE the \\0 subfolder got added by mapper
	std::wstring cmdline = StdUtility::string_format(L"model_aligner --input_path \"%s\\0\" --ref_images_path \"%s\\geo.txt\" --output_path \"%s\\GeoModel\" --ref_is_gps 0 --alignment_type enu --alignment_max_error 15.0", sparseFolder.c_str(), workingFolder.c_str(), sparseFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(9, "COLMAP: model_aligner");
	executeColmap(opt.exes.colmap_EXE, cmdline);
	

	// make the TXT version of the GeoModel model so we can read it.

	cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type TXT", geoFolder.c_str(), geoFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(9, "COLMAP: model_converter");
	executeColmap(opt.exes.colmap_EXE, cmdline);

	writeSparsePts(opt);
	if (opt.gType == MvsToSLPK_Options::GenerationType::SPARSE)
		return true;

	/// UNDISTORT

	// undistort will not rewrite the camera if there is no distortion!!  This is bad
	// even if the image size changes it will keep the wrong camera
	// have to force some distortion  used SIMPLE_RADIA
	
	cmdline = StdUtility::string_format(L"image_undistorter --image_path \"%s\" --input_path \"%s\\GeoModel\" --output_path \"%s\\dense\" --output_type COLMAP --max_image_size %d", imagesFolder.c_str(), sparseFolder.c_str(), workingFolder.c_str(), opt.distortMaxImageSize);

	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(12, "COLMAP: image_undistorter");
	executeColmap(opt.exes.colmap_EXE, cmdline);


	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";
	sparseFolder = denseFolder;
	sparseFolder += L"\\sparse";
	cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type TXT", sparseFolder.c_str(), sparseFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(12, "COLMAP: model_converter");
	executeColmap(opt.exes.colmap_EXE, cmdline);

	if (opt.fixedCameraParameters) {
		std::vector<std::wstring> foundFilenamesBin;
		StdUtility::findFiles(foundFilenamesBin, sparseFolder, L".*\\.bin", false);
		for (auto& binFile : foundFilenamesBin) {
			StdUtility::deleteFile(binFile);
		}

		overwriteDenseCam_UndistortFIX(opt);

		cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type BIN", sparseFolder.c_str(), sparseFolder.c_str());
		std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		executeColmap(opt.exes.colmap_EXE, cmdline);
	}
	
	return true;
}


static bool runCOLMAP(MvsToSLPK_Options& opt, bool endAtSpase)
{
	//"%COLMAP_EXE%" feature_extractor --database_path "%OUT_PATH%\database.db" --image_path "%DATASET_PATH%"\

	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring imageListTxt = StdUtility::convert(opt.imageListTxt);
	
	std::wstring cmdline = StdUtility::string_format(L"feature_extractor --database_path \"%s\\database.db\" --image_path \"%s\" --image_list_path \"%s\"", workingFolder.c_str(), imagesFolder.c_str(), imageListTxt.c_str());

	if (opt.camList.size() == 1) {
		cmdline += L" --ImageReader.single_camera 1";
	}

	if (opt.hasCamera) {
		// 			# Camera list with one line of data per camera :
		// 			#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]
		// 				# Number of cameras : 3
		// 				1 SIMPLE_PINHOLE 3072 2304 2559.81 1536 1152
		// 				2 PINHOLE 3072 2304 2560.56 2560.56 1536 1152
		// 				3 SIMPLE_RADIAL 3072 2304 2559.69 1536 1152 - 0.0218531
		// 
		//# Camera ID, Model, Width, Height, fx, fy, cx, cy, k1, k2, p1, p2
		//1 OPENCV 1920 1080 1000 1000 960 540 0.01 - 0.01 0.001 0.001
		//# Camera ID, Model, Width, Height, fx, fy, cx, cy, k1, k2, p1, p2, k3, k4, k5, k6, s1, s2, s3, s4, tx, ty
		//1 FULL_OPENCV 1920 1080 1000 1000 960 540 0.01 -0.01 0.001 0.001 0.0001 0.00001 -0.00001 0.00002 0.0 0.0 0.0 0.0 0.0005 0.0005


					// it is very important that only one camera is used!! OpenMVS Cuda will fail if any undistored image is not the same size
					// If more than one camera is used then the sizes can change!  This messes up OpenMVS
					// https://github.com/cdcseacave/openMVS/issues/750
					// 
					// USE this
					//--ImageReader.single_camera 1

					// change command line
		cmdline = StdUtility::string_format(L"feature_extractor --database_path \"%s\\database.db\" --image_path \"%s\" --image_list_path \"%s\" --ImageReader.camera_model SIMPLE_RADIAL --ImageReader.camera_params \"%.2f, %.2f, %.2f, %.7f\"",
			workingFolder.c_str(), imagesFolder.c_str(), imageListTxt.c_str(), opt.camList[0].camParams.focal, opt.camList[0].camParams.ppX, opt.camList[0].camParams.ppY, 0.0);

		if (opt.camList[0].camParams.hasDistortion) {
			iSMTPRJ::CameraParameters& cp = opt.camList[0].camParams;

// 			cmdline = StdUtility::string_format(L"feature_extractor --database_path \"%s\\database.db\" --image_path \"%s\" --image_list_path \"%s\" --ImageReader.camera_model FULL_OPENCV --ImageReader.camera_params \"%.2f, %.2f, %.2f, %.2f %g %g %g %g %g %g %g %g %g %g %g %g %g %g\"",
// 				workingFolder.c_str(), imagesFolder.c_str(), imageListTxt.c_str(), cp.focal, cp.focal, cp.ppX, cp.ppY, cp.k[1], cp.k[2], cp.p[0], cp.p[1], cp.k[3], cp.k[4], cp.k[5], cp.k[6], cp.b[0], cp.b[1], 0.0, 0.0, 0.0, 0.0);

			cmdline = StdUtility::string_format(L"feature_extractor --database_path \"%s\\database.db\" --image_path \"%s\" --image_list_path \"%s\" --ImageReader.camera_model FULL_OPENCV --ImageReader.camera_params \"%.2f, %.2f, %.2f, %.2f, %g, %g, %g, %g, %g, %g, %g, %g\"",
				workingFolder.c_str(), imagesFolder.c_str(), imageListTxt.c_str(), cp.focal, cp.focal, cp.ppX, cp.ppY, cp.k[1], cp.k[2], cp.p[0], cp.p[1], cp.k[3], cp.k[4], cp.k[5], cp.k[6]);
		}

		if (opt.camList.size() == 1) {
			cmdline += L" --ImageReader.single_camera 1";
		}

		// don't do this, camera must have some sort of distortion or undistort will not work...see comments below
// 			if (opt.fixedCameraParameters) {
// 				// had to switch to pinhole when using a fixed camera
// 				cmdline = StdUtility::string_format(L"feature_extractor --database_path \"%s\\database.db\" --image_path \"%s\" --image_list_path \"%s\" --ImageReader.single_camera 1 --ImageReader.camera_model PINHOLE --ImageReader.camera_params \"%f, %f, %f, %f\"",
// 					workingFolder.c_str(), imagesFolder.c_str(), imageListTxt.c_str(), opt.camParams.focal, opt.camParams.focal, opt.camParams.ppX, opt.camParams.ppY);
// 			}
	}

	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(7, "COLMAP: feature_extractor");
	executeColmap(opt.exes.colmap_EXE, cmdline);
	

	
	cmdline = StdUtility::string_format(L"exhaustive_matcher --database_path \"%s\\database.db\"", workingFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(8, "COLMAP: exhaustive_matcher");
	executeColmap(opt.exes.colmap_EXE, cmdline);
	

	if (endAtSpase)
		return true;

	std::wstring sparseFolder = workingFolder;
	sparseFolder += L"\\sparse";
	StdUtility::createFullDirectoryPath(sparseFolder);

	//"%COLMAP_EXE%" mapper --database_path "%OUT_PATH%\database.db" --image_path "%DATASET_PATH%" --output_path "%OUT_PATH%/sparse"
	
	cmdline = StdUtility::string_format(L"mapper --database_path \"%s\\database.db\" --image_path \"%s\" --output_path \"%s\"", workingFolder.c_str(), imagesFolder.c_str(), sparseFolder.c_str());

	// camera not allowed to change
	if (opt.fixedCameraParameters) {
		// we go ahead and allow --Mapper.ba_refine_extra_params 1 because we want to have a little distortion calculated
		// undistort step needs this or it will fail!!!  The camera will not update unless some distortion was found!
		// -- you might think that in feature_extractor you can give it a very small distortion param - THIS DOES not work.  It always set it 0 - i think it
		// reads as integers on the command 

		// actually its been found that setting all to Zer0 give the most accurate result!
		// So we fix the camera generated by Distortion instead and this works great
		// See function overwriteDenseCam

		if (opt.camList.size() == 1) {
			//cmdline += L" --Mapper.ba_refine_focal_length 0 --Mapper.ba_refine_principal_point 0 --Mapper.ba_refine_extra_params 0";
			cmdline += L" --Mapper.ba_refine_focal_length 0 --Mapper.ba_refine_principal_point 0 --Mapper.ba_refine_extra_params 0";
		}
	}

	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(9, "COLMAP: mapper");

	executeColmap(opt.exes.colmap_EXE, cmdline);
	

	// make the TXT version of the model so we can read it.
	
		// NOTE the \\0 subfolder got added by mapper
	cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\\0\" --output_path \"%s\\0\" --output_type TXT", sparseFolder.c_str(), sparseFolder.c_str());
	std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	//std::wstring cmdline;
	postProgress(9, "COLMAP: model_converter");
	executeColmap(opt.exes.colmap_EXE, cmdline);
	

	runCOLMAP2(opt);

	return true;
}

static bool runCOLMAP_fixedCameras(MvsToSLPK_Options& opt)
{
	// this just runs feature_extractor and exhaustive_matcher
	runCOLMAP(opt, true);


	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring imageListTxt = StdUtility::convert(opt.imageListTxt);

	std::wstring sparseFolder = workingFolder;
	sparseFolder += L"\\sparse";
	StdUtility::createFullDirectoryPath(sparseFolder);

	std::wstring geoTemFolder = sparseFolder;
	geoTemFolder += L"\\GeoModelTemp";
	StdUtility::createFullDirectoryPath(geoTemFolder);

	std::wstring geoFolder = sparseFolder;
	geoFolder += L"\\GeoModel";
	StdUtility::createFullDirectoryPath(geoFolder);

	// rearrange the opt.eoInfos to the order of database.db
	// use db to get the order
	std::string dbFile = opt.workingFolder;
	StdUtility::appendSlash(dbFile);
	dbFile += "database.db";
	rearrageImageOrderDB(opt, dbFile);


	std::string extOutFolder = StdUtility::convert(geoTemFolder);
	ExteriorsToColmap(opt, extOutFolder);


	{
		std::wstring cmdline = StdUtility::string_format(L"point_triangulator --database_path \"%s\\database.db\" --image_path \"%s\" --input_path \"%s\"  --output_path \"%s\"", workingFolder.c_str(), imagesFolder.c_str(), geoTemFolder.c_str(), geoFolder.c_str());

		// does not see to work the camera got adjusted.
		cmdline += L" --Mapper.ba_refine_focal_length 0 --Mapper.ba_refine_principal_point 0 --Mapper.ba_refine_extra_params 0";

		std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		
		executeColmap(opt.exes.colmap_EXE, cmdline);

		cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type TXT", geoFolder.c_str(), geoFolder.c_str());
		std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		//std::wstring cmdline;
		executeColmap(opt.exes.colmap_EXE, cmdline);

		writeSparsePts(opt);
		if (opt.gType == MvsToSLPK_Options::GenerationType::SPARSE)
			return true;
	}

	/// UNDISTORT

	// undistort will not rewrite the camera if there is no distortion!!  This is bad
	// even if the image size changes it will keep the wrong camera
	// have to force some distortion  used SIMPLE_RADIA
	{
		std::wstring cmdline = StdUtility::string_format(L"image_undistorter --image_path \"%s\" --input_path \"%s\\GeoModel\" --output_path \"%s\\dense\" --output_type COLMAP --max_image_size %d", imagesFolder.c_str(), sparseFolder.c_str(), workingFolder.c_str(), opt.distortMaxImageSize);

		std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		//std::wstring cmdline;
		executeColmap(opt.exes.colmap_EXE, cmdline);


		std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
		denseFolder += L"\\dense";
		std::wstring sparseFolder = denseFolder;
		sparseFolder += L"\\sparse";
		cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type TXT", sparseFolder.c_str(), sparseFolder.c_str());
		std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		//std::wstring cmdline;
		executeColmap(opt.exes.colmap_EXE, cmdline);

		// we need to rewrite the camera since image_undistorter does not do it unless the camera was updated
		// must be PINHOLE
		if (opt.fixedCameraParameters) {

			if (opt.camList.size() > 0 && !opt.camList[0].camParams.hasDistortion) {
				std::vector<std::wstring> foundFilenamesBin;
				StdUtility::findFiles(foundFilenamesBin, sparseFolder, L".*\\.bin", false);
				for (auto& binFile : foundFilenamesBin) {
					StdUtility::deleteFile(binFile);
				}

				overwriteDenseCam_UndistortFIX(opt);

				cmdline = StdUtility::string_format(L"model_converter --input_path \"%s\" --output_path \"%s\" --output_type BIN", sparseFolder.c_str(), sparseFolder.c_str());
				std::cout << StdUtility::convert(opt.exes.colmap_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
				executeColmap(opt.exes.colmap_EXE, cmdline);
			}
		}
	}

	return true;
}

static bool mvsToResults(MvsToSLPK_Options& opt)
{
	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";

//
// copy stuff to results folder
//

	// copy the dense point cloud...convert to las?
	std::string pointCloudFile = StdUtility::convert(denseFolder);
	StdUtility::appendSlash(pointCloudFile);
	pointCloudFile += "scene_dense.ply"; // PLY not geo reference so useless...convert to LAS

	// ply to las...
	std::string pointCloudFileResult = opt.resultsFolder;
	StdUtility::appendSlash(pointCloudFileResult);
	pointCloudFileResult += "DensePointCloud.las"; // PLY not geo reference so useless...convert to LAS

	PlyFileToLAS(pointCloudFile, pointCloudFileResult, opt.boxFilename);


	//
	// copy Textured Mesh file
	//
	std::wstring MeshFile = denseFolder;
	StdUtility::appendSlash(MeshFile);
	MeshFile += L"scene_dense_texture.ply";

	std::wstring resultMesh = StdUtility::convert(opt.resultsFolder);
	StdUtility::appendSlash(resultMesh);
	resultMesh += L"Mesh";
	StdUtility::createFullDirectoryPath(resultMesh);
	resultMesh += L"\\scene_dense_texture.ply";

	StdUtility::copyFile(MeshFile, resultMesh);

	// copy PNG file

	std::wstring texFile = StdUtility::replaceExtension(MeshFile, L".png");
	bool hasDefaultTextureFile = StdUtility::fileExists(texFile);

	if (!hasDefaultTextureFile) {
		// try the png with a 0 on the end
		texFile = StdUtility::appendName(texFile, L"0");
		hasDefaultTextureFile = StdUtility::fileExists(texFile);

		if (!hasDefaultTextureFile) {
			// still not found then read the PLY and see if it is there..
		}
	}

	// the texture file can get really big so it is reduced to a max size
	resizeTexturePow2(StdUtility::convert(texFile));

	std::wstring resultTexFile = StdUtility::convert(opt.resultsFolder);
	StdUtility::appendSlash(resultTexFile);
	resultTexFile += L"Mesh\\";
	resultTexFile += StdUtility::getName(texFile, false);

	StdUtility::copyFile(texFile, resultTexFile);

	// copy the box file here too...

	std::string resultBoxFile = opt.resultsFolder;
	StdUtility::appendSlash(resultBoxFile);
	resultBoxFile += "Mesh\\";
	resultBoxFile += StdUtility::getName(opt.boxFilename, false);

	StdUtility::copyFile(opt.boxFilename, resultBoxFile);

	return true;
}

static bool mvsToResultsMulti(MvsToSLPK_Options& opt)
{
	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";


	std::string pointCloudFileResult = opt.resultsFolder;
	StdUtility::appendSlash(pointCloudFileResult);
	pointCloudFileResult += "DensePointCloud.las"; // PLY not geo reference so useless...convert to LAS

	// make a single LAS file from all the PLY files and put in results
	PlyFileToLAS(opt.densePtsPlyResults, pointCloudFileResult, opt.boxFilename);

	for (auto& plyFile : opt.texturePlyResults) {
		//
		// copy TEXTURED Mesh file
		//

		std::wstring resultMesh = StdUtility::convert(opt.resultsFolder);
		StdUtility::appendSlash(resultMesh);
		resultMesh += L"Mesh";
		StdUtility::createFullDirectoryPath(resultMesh);

		// same name, new folder
		resultMesh += L"\\";
		resultMesh += StdUtility::getName(plyFile, false);

		StdUtility::copyFile(plyFile, resultMesh);

		// copy PNG file

		std::wstring texFile = StdUtility::replaceExtension(plyFile, L".png");
		bool hasDefaultTextureFile = StdUtility::fileExists(texFile);

		if (!hasDefaultTextureFile) {
			// try the png with a 0 on the end
			texFile = StdUtility::appendName(texFile, L"0");
			hasDefaultTextureFile = StdUtility::fileExists(texFile);

			if (!hasDefaultTextureFile) {
				// still not found then read the PLY and see if it is there..
			}
		}

		// the texture file can get really big so it is reduced to a max size
		resizeTexturePow2(StdUtility::convert(texFile));

		std::wstring resultTexFile = StdUtility::convert(opt.resultsFolder);
		StdUtility::appendSlash(resultTexFile);
		resultTexFile += L"Mesh\\";
		resultTexFile += StdUtility::getName(texFile, false);

		StdUtility::copyFile(texFile, resultTexFile);
	}


	// only one box file...
	// copy the boxfile here too...
	std::string resultBoxFile = opt.resultsFolder;
	StdUtility::appendSlash(resultBoxFile);
	resultBoxFile += "Mesh\\";
	resultBoxFile += StdUtility::getName(opt.boxFilename, false);

	StdUtility::copyFile(opt.boxFilename, resultBoxFile);

	return true;
}

// we may have to do our own pre processing to make all 
// images the same size, 8-bit, and distortion free, and in same folder

// keep getting CUDA error: invalid argument (code 1) - making sure image size is the same seemed to fix
// but then the error re - occurred.
// turned off CUDA for DensifyPointCloud, need to set --max-threads to 8 or something
// Don't think we can go back to using CUDA...without debugging DensifyPointCloud
// could run Cuda first and if error revert to CPU?

#if 0

static bool runOpenMvs(MvsToSLPK_Options& opt)
{
	// so machine does not become basically unusable
	// can be set to 0 to use all available cores
	unsigned int maxThreads = std::max(unsigned int(2), opt.cpu_cores / 2); // at least 2 or more, but typically use half the cores
	std::cout << "MVS: using " << maxThreads << " threads." << std::endl;

	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";

	// makes the .mvs file
	{
		std::wstring cmdline = StdUtility::string_format(L"-i \"%s\" -o \"%s\\scene.mvs\" -w \"%s\" --image-folder \"%s\\images\"", denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str());
		std::cout << StdUtility::convert(opt.exes.InterfaceCOLMAP_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.InterfaceCOLMAP_EXE, cmdline);
	}

	std::wstring testIt;
	testIt = denseFolder;
	StdUtility::appendSlash(testIt);
	testIt += L"scene.mvs";
	if (!StdUtility::fileExists(testIt)) {
		spdlog::error("Unable to make scene.mvs");
		return false;
	}
	
	{
		// CUDA removed much slower lower but doesn't fail
		// safer to just not use it
		// also setting max-threads to 8 so the CPU does not go to 100%
		std::wstring cmdline = StdUtility::string_format(L"-i \"%s\\scene.mvs\" -w \"%s\" --cuda-device -2 --max-threads %u", denseFolder.c_str(), denseFolder.c_str(), maxThreads);
		std::cout << StdUtility::convert(opt.exes.DensifyPointCloud_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.DensifyPointCloud_EXE, cmdline);
	}

	testIt = denseFolder;
	StdUtility::appendSlash(testIt);
	testIt += L"scene_dense.mvs";
	if (!StdUtility::fileExists(testIt)) {
		spdlog::error("Unable to make scene_dense.mvs");
		return false;
	}

	{
		std::wstring cmdline = StdUtility::string_format(L"-i \"%s\\scene_dense.mvs\" -w \"%s\" --max-threads %u", denseFolder.c_str(), denseFolder.c_str(), maxThreads);
		std::cout << StdUtility::convert(opt.exes.ReconstructMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.ReconstructMesh_EXE, cmdline);
	}

	{
		// This is definitely the slow and dangerous part - uses lot of memory
		// how much better is the mesh after this
		// --resolution-level 1  increase to use less mem ?
		// --use-cuda 1

		if (opt.refineMesh) {
			std::wstring cmdline = StdUtility::string_format(L"-i \"%s\\scene_dense.mvs\" -m \"%s\\scene_dense_mesh.ply\" -w \"%s\" --resolution-level 1 --max-threads %u", denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str(), maxThreads);
			std::cout << StdUtility::convert(opt.exes.RefineMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
			execute(opt.exes.RefineMesh_EXE, cmdline);
		}
		else {
			// just copy the Mesh to refined mesh...even though not refined.
			std::wstring MeshFile = denseFolder;
			StdUtility::appendSlash(MeshFile);
			MeshFile += L"scene_dense_mesh.ply";
			std::wstring resultMesh = StdUtility::convert(opt.resultsFolder);
			StdUtility::appendSlash(resultMesh);
			resultMesh += L"scene_dense_refine.ply";

			StdUtility::copyFile(MeshFile, resultMesh);
		}
	}

	{
		std::wstring cmdline = StdUtility::string_format(L"-i \"%s\\scene_dense.mvs\" -m \"%s\\scene_dense_refine.ply\" -w \"%s\" --max-texture-size 0 --empty-color 0 --max-threads %u --cost-smoothness-ratio 1", denseFolder.c_str(), denseFolder.c_str(), denseFolder.c_str(), maxThreads);
		std::cout << StdUtility::convert(opt.exes.textureMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.textureMesh_EXE, cmdline);

		// it is possible that textureMesh.exe will make a gigantic Texture  we have to use a single at least for now...
	}

	mvsToResults(opt);

	return true;
}
#endif

static bool mvsProcessDenseMvs(MvsToSLPK_Options& opt, std::wstring &densMVS, unsigned int maxThreads)
{
	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";

	if (!StdUtility::fileExists(densMVS)) {
		spdlog::error("Unable to make scene_dense.mvs");
		return false;
	}

	// add the dense PLY to results
	std::wstring densePly = StdUtility::replaceExtension(densMVS, L"ply");
	opt.densePtsPlyResults.push_back(densePly);


	std::wstring denseName = StdUtility::getName(densMVS);


	std::wstring cmdline = StdUtility::string_format(L"-i \"%s\" -w \"%s\" --max-threads %u --close-holes 200 --smooth 10", densMVS.c_str(), denseFolder.c_str(), maxThreads);
	std::cout << StdUtility::convert(opt.exes.ReconstructMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	execute(opt.exes.ReconstructMesh_EXE, cmdline);

	postProgress(40, "Refine Mesh");

	// This is definitely the slow and dangerous part - uses lot of memory
	// how much better is the mesh after this
	// --resolution-level 1  increase to use less mem ?
	// --use-cuda 1

	if (opt.refineMesh) {
		std::wstring cmdline = StdUtility::string_format(L"-i \"%s\" -m \"%s\\%s_mesh.ply\" -w \"%s\" --resolution-level 1 --max-threads %u", densMVS.c_str(), denseFolder.c_str(), denseName.c_str(), denseFolder.c_str(), maxThreads);
		std::cout << StdUtility::convert(opt.exes.RefineMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.RefineMesh_EXE, cmdline);
	}
	else {
		// just copy the Mesh to refined mesh...even though not refined.
		std::wstring MeshFile = denseFolder;
		StdUtility::appendSlash(MeshFile);
		MeshFile += denseName;
		MeshFile += L".ply";
		std::wstring resultMesh = StdUtility::convert(opt.resultsFolder);
		StdUtility::appendSlash(resultMesh);
		resultMesh += denseName;
		resultMesh += L"_refine.ply";

		StdUtility::copyFile(MeshFile, resultMesh);
	}

	postProgress(65, "Texture Mesh");

	cmdline = StdUtility::string_format(L"-i \"%s\\%s.mvs\" -m \"%s\\%s_refine.ply\" -w \"%s\" --max-texture-size 0 --empty-color 0 --max-threads %u  --cost-smoothness-ratio 1 --resolution-level 2", denseFolder.c_str(), denseName.c_str(), denseFolder.c_str(), denseName.c_str(), denseFolder.c_str(), maxThreads);
	std::cout << StdUtility::convert(opt.exes.textureMesh_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
	execute(opt.exes.textureMesh_EXE, cmdline);

	std::wstring resultTexturePly = denseFolder;
	StdUtility::appendSlash(resultTexturePly);
	resultTexturePly += denseName;
	resultTexturePly += L"_texture.ply";
	//scene_0000_dense_texture.ply
	if (StdUtility::fileExists(resultTexturePly)) {
		// add to results.
		opt.texturePlyResults.push_back(resultTexturePly);
	}

	postProgress(75);

	return true;
}

// need to calculate this, see copilot
static unsigned int calculateSubScene(MvsToSLPK_Options& opt)
{
	//return 2000;
	return 660000 * 2;
	//return 600;
}


// spliting subscene
// https://github.com/cdcseacave/openMVS/issues/438


static bool runOpenMvsWSplit(MvsToSLPK_Options& opt, bool splitUp)
{
	// so machine does not become basically unusable
	// can be set to 0 to use all available cores
	unsigned int maxThreads = std::max(unsigned int(2), opt.cpu_cores / 2); // at least 2 or more, but typically use half the cores
	std::cout << "MVS: using " << maxThreads << " threads." << std::endl;

	std::wstring workingFolder = StdUtility::convert(opt.workingFolder);
	std::wstring imagesFolder = StdUtility::convert(opt.imagesFolder);
	std::wstring denseFolder = StdUtility::convert(opt.workingFolder);
	denseFolder += L"\\dense";


	//colmapToScene(opt);
	
	postProgress(25);

	std::wstring cmdline;

	std::wstring testIt;
	testIt = denseFolder;
	StdUtility::appendSlash(testIt);
	testIt += L"scene.mvs";
	if (!StdUtility::fileExists(testIt)) {
		spdlog::error("Unable to make scene.mvs");
		return false;
	}


	//--resolution - level arg(= 1)

	// CUDA removed much slower lower but doesn't fail
	// safer to just not use it
	// also setting max-threads to 8 so the CPU does not go to 100%
	unsigned int subSceneArea = calculateSubScene(opt);

	//int cudaDevVal = -2; // -2 means no CUDA
	int cudaDevVal = -1; // means best CUDA

	int resLevDensifyPT = 2;

	if (splitUp) {
		// write the Densify.ini file
		std::wstring densIni = denseFolder;
		StdUtility::appendSlash(densIni);
		densIni += L"Densify.ini";
		std::ofstream outFile;
		outFile.open(densIni);
		if (!outFile.is_open()) {
			return false;
		}
		outFile << "Optimize = 0" << std::endl;
		outFile << "Estimation Geometric Iters = 0" << std::endl;
		outFile.close();

		// call with --sub-scene-area to split things up. this should make some scene_0000.mvs files?
		// got chopped up nicely...now what

		// lets put the chopped up ones in a sub folder
// 		std::wstring chopFolder = denseFolder;
// 		chopFolder += L"\\chopped";
// 		StdUtility::createFullDirectoryPath(chopFolder);

		// the first step is to compute all depth maps without fusion
		// this does not use a ton of memory hence does not need split, creates a bunch of .dmap files

		
#if 1
		// notice now DensifyPointCloud is called three times.  Usually without spliting it is called once.
		// 1. creates dmaps
		// 2. splits it up into many scence can get 1..n named like: scene_0000.mvs
		// 3. DensifyPointCloud called for each scene created in 2.

		postProgress(25, "DensifyPointCloud");

		// just create the .dmaps 
		cmdline = StdUtility::string_format(L"-i \"%s\\scene.mvs\"  --fusion-mode 1 -w \"%s\" --cuda-device %d --max-threads %u --resolution-level %d", denseFolder.c_str(), denseFolder.c_str(), cudaDevVal, maxThreads, resLevDensifyPT);
		std::cout << StdUtility::convert(opt.exes.DensifyPointCloud_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		int result = execute(opt.exes.DensifyPointCloud_EXE, cmdline);
		if (result != 0) {
			std::cout << "DensifyPointCloud exited with " << result << std::endl;
			cudaDevVal = -2;
			cmdline = StdUtility::string_format(L"-i \"%s\\scene.mvs\"  --fusion-mode 1 -w \"%s\" --cuda-device %d --max-threads %u --resolution-level %d", denseFolder.c_str(), denseFolder.c_str(), cudaDevVal, maxThreads, resLevDensifyPT);
			result = execute(opt.exes.DensifyPointCloud_EXE, cmdline);
			//cudaDevVal = -1;
			// if it fails the other step will too
		}

		// split the scene into sub scenes
		cmdline = StdUtility::string_format(L"-i \"%s\\scene.mvs\"  --sub-scene-area %u -w \"%s\" --cuda-device %d --max-threads %u --resolution-level %d", denseFolder.c_str(), subSceneArea, denseFolder.c_str(), cudaDevVal, maxThreads, resLevDensifyPT);
		std::cout << StdUtility::convert(opt.exes.DensifyPointCloud_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		result = execute(opt.exes.DensifyPointCloud_EXE, cmdline);
		if (result != 0) {
			std::cout << "DensifyPointCloud Step 2 exited with " << result << std::endl;
		}
#endif

		postProgress(30);

		// now what to do with the chopped up
		// find what got generated
		//DensifyPointCloud scene_0000.mvs --dense-config-file Densify.ini --max-resolution 800 --number-views-fuse 2 --max-threads 4 -w "sheffield_cross/opensfm/undistorted/openmvs/depthmaps" -v 0
		std::vector<std::wstring> foundFilenames;
		StdUtility::findFiles(foundFilenames, denseFolder, L"scene_[0-9][0-9][0-9][0-9]\\.mvs", false);

		// call DensifyPointCloud on each new scene
		for (auto& mvsSubFile : foundFilenames) {
			cmdline = StdUtility::string_format(L"-i \"%s\" --dense-config-file \"%s\" -w \"%s\" --cuda-device %d --max-threads %u --number-views-fuse 4 --resolution-level %d", mvsSubFile.c_str(), densIni.c_str(), denseFolder.c_str(), cudaDevVal, maxThreads, resLevDensifyPT);
			std::cout << StdUtility::convert(opt.exes.DensifyPointCloud_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
			execute(opt.exes.DensifyPointCloud_EXE, cmdline);
		}
		
		postProgress(35);

		// takes a dense version MVS, like mvs scene_0004_dense.mvs and finishes processing it.
		for (auto& mvsSubFile : foundFilenames) {
			std::wstring densMVS = StdUtility::appendName(mvsSubFile, L"_dense");
			mvsProcessDenseMvs(opt, densMVS, maxThreads);
		}

	}
	else {
		cmdline = StdUtility::string_format(L"-i \"%s\\scene.mvs\" -w \"%s\" --cuda-device %d --max-threads %u --number-views-fuse 4 --resolution-level %d", denseFolder.c_str(), denseFolder.c_str(), cudaDevVal, maxThreads, resLevDensifyPT);
		std::cout << StdUtility::convert(opt.exes.DensifyPointCloud_EXE.c_str()) << " " << StdUtility::convert(cmdline).c_str() << std::endl;
		execute(opt.exes.DensifyPointCloud_EXE, cmdline);

		postProgress(35);

		std::wstring densMVS = denseFolder;
		StdUtility::appendSlash(densMVS);
		densMVS += L"scene_dense.mvs";
		postProgress(25, "DensifyPointCloud");
		mvsProcessDenseMvs(opt, densMVS, maxThreads);
	}


	mvsToResultsMulti(opt);

	return true;
}

static void makeSplitFiles(MvsToSLPK_Options& opt, std::vector<std::vector<std::string>>& lodSplitfiles)
{
	std::string workingFolder = opt.workingFolder;
	std::string denseFolder = opt.workingFolder;
	denseFolder += "\\dense";

	auto slpkDll = std::shared_ptr<SLPK3DTiles>(createSLPK3DTiles(), [](SLPK3DTiles* p) { p->Release(); });

	int count = 1;
	for (auto& plyFile : opt.texturePlyResults) {
		SplitFilesOptions sfOpt;
		sfOpt.inputMeshFile = StdUtility::convert(plyFile);
		sfOpt.workingFolder = workingFolder;
		StdUtility::appendSlash(sfOpt.workingFolder);
		sfOpt.workingFolder += "Splits";
		sfOpt.workingFolder += StdUtility::string_format("_%d", count);
		StdUtility::createFullDirectoryPath(sfOpt.workingFolder);
		sfOpt.splitDivisions = 1;

		slpkDll->generateSplitFiles(sfOpt);

		// returns a list of sfOpt.lodSplitfiles;
		if (sfOpt.lodSplitfiles.size() > 0) {
			opt.vectorOfSplitFiles.push_back(sfOpt.lodSplitfiles);
		}

		++count;
	}
}

static bool generateSLPK(MvsToSLPK_Options& opt, std::vector<std::vector<std::string>>& lodSplitfiles, std::shared_ptr<iSMTPRJ>& smtPrj)
{
	std::string workfolder = opt.workingFolder;

	std::string boxFilename = workfolder;
	StdUtility::appendSlash(boxFilename);
	boxFilename += "geo.box";

	if (!StdUtility::fileExists(boxFilename)) {
		std::cout << "Box file missing: " << boxFilename.c_str() << std::endl;
		return false;
	}

 	auto slpkDll = std::shared_ptr<SLPK3DTiles>(createSLPK3DTiles(), [](SLPK3DTiles* p) { p->Release(); });


	int count = 0;
	for (auto& lodFiles : opt.vectorOfSplitFiles) {
		SlpkOptions sOpt;
		sOpt.lodSplitfiles = lodFiles;
		sOpt.boxFilename = boxFilename;
		sOpt.prjFilename = opt.prjFile; // must be provided

		if (!StdUtility::fileExists(opt.prjFile)) {
			// use Summit project to convert coordinates instead
			//sOpt._smtxml = smtPrj.get();
		}

		std::string name = StdUtility::getName(opt.summitProject);
		std::replace(name.begin(), name.end(), ' ', '_');

		sOpt.ouput3DTileFolder = opt.resultsFolder;
		StdUtility::appendSlash(sOpt.ouput3DTileFolder);
		sOpt.ouput3DTileFolder += "3DTiles";
		sOpt.ouput3DTileFolder += "_";
		sOpt.ouput3DTileFolder += name;
		sOpt.ouput3DTileFolder += StdUtility::string_format("_%d", count);
		if (opt.generate3DTiles)
			slpkDll->generate3DTiles(sOpt);


		sOpt.outSlpkFilename = opt.resultsFolder;
		StdUtility::appendSlash(sOpt.outSlpkFilename);
		sOpt.outSlpkFilename += name;
		sOpt.outSlpkFilename += StdUtility::string_format("_%d", count);
		sOpt.outSlpkFilename += ".slpk";

		if (opt.generateSLPK)
			slpkDll->generateSLPK(sOpt);

		++count;
	}

	return true;
}

