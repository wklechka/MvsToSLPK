#pragma once
#include <string>
#include <vector>

class DecimatorOptions
{
public:
	std::string inputMeshFile;
	std::string workingFolder; // folder to place split files...

	int numLevels = 3;
	int splitDivisions = 2;
	bool forceSquareTiles = true;
};

bool createDecimationLevelsVCG(DecimatorOptions& options, std::vector<std::vector<std::string>>& lodSplitfiles);