#include "pch.h"
#include "CreateLOD.h"

#include "StdUtil\StdUtility.h"
#include "SplitName.h"
#include "recursiveSplit.h"
//#include "VCGMesh.h"
#include "i3s\i3s_common_.h"
#include "i3s/i3s_writer_impl.h"
#include "i3s/i3s_legacy_mesh.h"
#include "i3s/i3s_writer.h"
#include "utils/utl_png.h"
#include "utils\utl_obb.h"
#include "i3s\i3s_common.h"

#include "TexFunc.h"

bool doObjSplitOnObj(const std::string& objFile, std::vector<std::string>& splitFiles, int splitDiv, bool forceSquareTiles, int lod)
{
	std::string dir = StdUtility::getDirectory(objFile);

	// see if split files already exist
	std::vector<std::string> foundFilenames;
	bool foundFiles = StdUtility::findFiles(foundFilenames, dir, SplitName_ SplitSearch, false);

	if (foundFilenames.size() == 0) {

		if (!splitUpMesh(objFile, dir, splitDiv, forceSquareTiles, lod)) {
			return false;
		}

		foundFiles = StdUtility::findFiles(foundFilenames, dir, SplitName_ SplitSearch, false);
		if (!foundFiles)
			return false;
	}

	splitFiles = foundFilenames;
	return true;
}
#if 0
bool decimateMesh(MyMesh& mesh, bool useFace = false, std::size_t divsor = 4)
{
	try {
		printf("Decimate Mesh: V: %d F: %d\n", mesh.vn, mesh.fn);
		int targetNum = mesh.fn / divsor;
		if (!useFace) {
			targetNum = mesh.vn / divsor;
		}

		vcg::tri::TriEdgeCollapseQuadricParameter qparams;
		qparams.QualityThr = .3;
		double TargetError = std::numeric_limits<double >::max();

		vcg::tri::UpdateBounding<MyMesh>::Box(mesh);

		// decimation initialization
		vcg::LocalOptimization<MyMesh> DeciSession(mesh, &qparams);

		int t1 = clock();
		DeciSession.Init<MyTriEdgeCollapse>();
		int t2 = clock();
		//printf("Initial Heap Size %i\n", int(DeciSession.h.size()));

		if (useFace) {
			DeciSession.SetTargetSimplices(targetNum);
		}
		else {
			DeciSession.SetTargetVertices(targetNum);
		}

		DeciSession.SetTimeBudget(0.5f);
		DeciSession.SetTargetOperations(100000);
		if (TargetError < std::numeric_limits<float>::max()) DeciSession.SetTargetMetric(TargetError);

		if (useFace) {
			while (DeciSession.DoOptimization() && mesh.fn > targetNum && DeciSession.currMetric < TargetError) {
				printf("Mesh Face size %7i heap sz %9i err %9g \n", mesh.fn, int(DeciSession.h.size()), DeciSession.currMetric);
			}
		}
		else {
			while (DeciSession.DoOptimization() && mesh.vn > targetNum && DeciSession.currMetric < TargetError) {
				printf("Mesh Vertex size %7i err %9g \n", mesh.vn, DeciSession.currMetric);
				//printf("Mesh Vertex size %7i heap sz %9i err %9g \n", mesh.vn, int(DeciSession.h.size()), DeciSession.currMetric);
			}
		}

		int t3 = clock();
		//printf("mesh  %d %d Error %g \n", mesh.vn, mesh.fn, DeciSession.currMetric);
		//printf("\nCompleted in (%5.3f+%5.3f) sec\n", float(t2 - t1) / CLOCKS_PER_SEC, float(t3 - t2) / CLOCKS_PER_SEC);

		// garbage collection
		vcg::tri::Allocator<MyMesh>::CompactFaceVector(mesh);
		vcg::tri::Allocator<MyMesh>::CompactVertexVector(mesh);

		printf("Decimate Mesh After: V: %d F: %d\n", mesh.vn, mesh.fn);
	}
	catch (...) {
		return false;
	}

	return true;
}
#endif




bool copyBoxAndProj(const std::string& ifname, std::string& boxFileResult, std::string& projFileResult)
{
	std::string splitDir = StdUtility::getDirectory(ifname);
	splitDir += SplitFolder;

	std::string outputFolder = splitDir;
	StdUtility::appendSlash(outputFolder);

	// BOX files...

	std::string boxFile = StdUtility::replaceExtension(ifname, "box");
	bool hasDefaultBoxFile = StdUtility::fileExists(boxFile);

	std::string boxFileName = boxFile;

	std::string boxOut = outputFolder;
	boxOut += "offset.box";
	StdUtility::copyFile(boxFileName, boxOut);

	std::string prj_file_name = ifname;
	prj_file_name += ".prj";

	std::string projOut = outputFolder;
	projOut += "mesh.prj";
	StdUtility::copyFile(prj_file_name, projOut);

	boxFileResult = boxOut;
	projFileResult = projOut;

	return true;
}

static bool readPNG(const std::string& textureFile, std::vector<uint8_t>& data, int& width, int& height)
{
	std::vector<char>& dataRef = *((std::vector<char>*) & data); // had to cast it
	if (!i3slib::utl::read_png_from_file(textureFile, &width, &height, &dataRef))
	{
		std::cout << "Failed to load color bitmap from file." << std::endl;
		return false;
	}

	//int size = width;

	// The current png reader implementation always produces RGBA output for color images.
	// We have to strip alpha channel here.
	I3S_ASSERT(data.size() == width * height * 4);
	data.resize(remove_alpha_channel(data.data(), data.size()));
	I3S_ASSERT(data.size() == width * height * 3);

	return true;
}



#if 0
bool createDecimationLevelsVCG(ProcessingOptions& options, int numLevels, std::vector<std::vector<std::string>>& lodSplitfiles)
{
	const int MIN_NUM_VERTICES = 2000;

	if (numLevels < 1)
		return false;

	std::string ifname = options.ifname;
	StdUtility::replaceAll(ifname, "\\", "/");

	// need to check for already done state...!!
	// mostly to test, but also if we want to gen both 3DTilse and SLPK in the same run
	std::string splitDir = StdUtility::getDirectory(ifname);
	splitDir += SplitFolder;
	std::vector<std::string> foundDir;
	bool foundDirs = StdUtility::findSubFoldersNamed(foundDir, splitDir, LODname_ ".*");

	if (foundDirs) {
		for (auto lodFolder : foundDir) {
			std::vector<std::string> foundSplitFiles;
			bool foundFiles = StdUtility::findFiles(foundSplitFiles, lodFolder, SplitName_ SplitSearch, false);
			lodSplitfiles.push_back(foundSplitFiles);
		}

		std::cout << "Split folder found. Skipping." << std::endl;

		return true;
	}


	std::vector<std::vector<uint8_t>> textures;
	std::vector<int> texturesSizes;
	std::string orginalTexFile;
	generateAllTextures(ifname, textures, texturesSizes, 256, &orginalTexFile);

	MyMesh mesh;
	int result = vcg::tri::io::Importer<MyMesh>::Open(mesh, ifname.c_str());


	const std::string fileName = LODname;

	std::vector<std::string> lodFolders;
	for (int lev = 0; lev < numLevels; ++lev) {

		std::cout << "Decimation level: " << lev << std::endl;

		std::string lodDir = StdUtility::getDirectory(ifname);
		lodDir += SplitFolder "/" LODname_;
		lodDir += std::to_string(lev);
		StdUtility::createFullDirectoryPath(lodDir);

		// copy a reduced by half texture...so reduce mesh and texture
		int depth = lev;
		auto& texture = (depth < textures.size()) ? textures[depth] : textures.back();
		auto& textureSize = (depth < texturesSizes.size()) ? texturesSizes[depth] : texturesSizes.back();

		std::string pngFile;
		pngFile = lodDir;
		pngFile += "/";
		pngFile += fileName;
		pngFile += ".png";

		if (lev == 0) { // purely just a speed boost
			StdUtility::copyFile(orginalTexFile, pngFile);
		}
		else {
			writePNG(pngFile, textureSize, textureSize, (const char*)texture.data(), texture.size());
		}


		std::string decFile;
		decFile = lodDir;
		decFile += "/";
		decFile += fileName;
		decFile += ".obj";
		// 2, 4, 8,...
		//int divisor = std::pow(2, (lev + 1));
		int divisor = 2;


		int mask = vcg::tri::io::Mask::IOM_WEDGTEXCOORD | vcg::tri::io::Mask::IOM_VERTCOORD | vcg::tri::io::Mask::IOM_FACEINDEX;

		std::string texture_file = "./";
		texture_file += StdUtility::getName(pngFile, false);

		mesh.textures.clear();
		mesh.textures.push_back(texture_file);
		vcg::tri::io::Exporter<MyMesh>::Save(mesh, decFile.c_str(), mask);


		lodFolders.push_back(lodDir);

		// only create the level if there are enough vertices
		// no point in decimating if very few vertices
		if (mesh.VN() < MIN_NUM_VERTICES) {
			std::cout << "Decimation skip: minimum vert: " << mesh.VN() << std::endl;
			break;
		}


		if (lev == (numLevels - 1)) {
			// avoid extra decimate
			break;
		}

		// decimate next level - the first level is not decimated

		decimateMesh(mesh, false, divisor);

	}

	// split each LOD

	std::cout << "Starting Split" << std::endl;

	int count = 0;
	for (auto lodDir : lodFolders) {
		std::cout << "Split LOD level: " << count << std::endl;

		std::string decFile;
		decFile = lodDir;
		decFile += "/";
		decFile += fileName;
		decFile += ".obj";

		std::vector<std::string> splitFiles;
		doObjSplitOnObj(decFile, splitFiles, options.splitDivisions, options.forceSquareTiles, count);

		lodSplitfiles.push_back(splitFiles);

		++count;
	}

	std::cout << "Split time: " << float(clock() - options.t1) / CLOCKS_PER_SEC << " seconds" << std::endl;

	return true;
}
#endif

// bool createDecimationLevels(ProcessingOptions& options)
// {
// 	bool status = createDecimationLevelsVCG(options, options.lod, options.lodSplitfiles);
// 	copyBoxAndProj(options.ifname, options.boxFileName, options.prjFilename);
// 	return status;
// }

