#pragma once
#if !defined(SLPK_DLL)
#  if defined(_WIN32) && defined(SLPK_BUILD_AS_DLL)
#    define SLPK_DLL     __declspec(dllexport)
#  else
#    define SLPK_DLL
#  endif
#endif

#include <stdint.h>
#include <vector>
#include <string>

struct SlpkOptions
{
	// the mesh files already in LOD
	std::vector<std::vector<std::string>> lodSplitfiles;
	std::string boxFilename;
	std::string prjFilename;

	// SLPK specific
	std::string outSlpkFilename;
	int slpkTileLimit = -1; // when to start generating multiple SLPKs

	// 3D Tile specific
	std::string ouput3DTileFolder;
};

struct SplitFilesOptions
{
	std::string inputMeshFile;
	std::string workingFolder; // folder to place split files...

	int numLevels = 3;
	int splitDivisions = 2;
	bool forceSquareTiles = true;

	// result
	std::vector<std::vector<std::string>> lodSplitfiles;
};

struct SLPK3DTiles
{
public:

	virtual void Release() = 0;

	virtual bool generateSLPK(SlpkOptions& opt) = 0;
	virtual bool generate3DTiles(SlpkOptions& opt) = 0;

	virtual bool generateSplitFiles(SplitFilesOptions& opt) = 0;
};



extern "C" SLPK_DLL SLPK3DTiles * createSLPK3DTiles();