#include "pch.h"
#include "Decimator.h"

#include "StdUtil\WinUtility.h"
#include "StdUtil\StdUtility.h"
#include "SplitName.h"
/*#include "recursiveSplit.h"*/ // cannot include this
#include "VCGMesh.h"
#include "CreateLOD.h"
#include "TexFunc.h"

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

#if 1

bool generateAllTextures(const std::string& ifname, std::vector<std::vector<uint8_t>>& textures, std::vector<int>& texturesSizes, int min_size, std::string* orgTextureFile)
{
	std::string texFile = StdUtility::replaceExtension(ifname, ".png");
	bool hasDefaultTextureFile = StdUtility::fileExists(texFile);

	if (!hasDefaultTextureFile) {
		// try the png with a 0 on the end
		texFile = StdUtility::appendName(texFile, "0");
		hasDefaultTextureFile = StdUtility::fileExists(texFile);

		if (!hasDefaultTextureFile) {
			// still not found then read the PLY and see if it is there...

		}
	}

	std::vector<uint8_t> texture;
	int width, height;
	if (!readPNG(texFile, texture, width, height)) {
		return false;
	}

	int imageSize = width;

	textures.emplace_back(std::move(texture));
	texturesSizes.push_back(imageSize);

	uint8_t* pixelData = textures[0].data();
	build_downsampled_textures(imageSize, pixelData, min_size, textures, texturesSizes);

	if (orgTextureFile) {
		*orgTextureFile = texFile;
	}

	return true;
}

bool createDecimationLevelsVCG(DecimatorOptions& options, std::vector<std::vector<std::string>>& lodSplitfiles)
{
	const int MIN_NUM_VERTICES = 2000;

	int numLevels = options.numLevels;

	if (numLevels < 1)
		return false;

	std::string ifname = options.inputMeshFile;
	StdUtility::replaceAll(ifname, "\\", "/");

	// need to check for already done state...!!
	// mostly to test, but also if we want to gen both 3DTilse and SLPK in the same run
	std::string splitDir = options.workingFolder;
	StdUtility::replaceAll(splitDir, "\\", "/");
	StdUtility::appendSlash(splitDir);
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

		std::string lodDir = options.workingFolder;
		StdUtility::appendSlash(lodDir);

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

	//std::cout << "Split time: " << float(clock() - options.t1) / CLOCKS_PER_SEC << " seconds" << std::endl;

	return true;
}

// bool createDecimationLevels(DecimatorOptions& options)
// {
// 	bool status = createDecimationLevelsVCG(options, options.lodSplitfiles);
// 	//copyBoxAndProj(options.ifname, options.boxFileName, options.prjFilename);
// 	return status;
// }

#endif