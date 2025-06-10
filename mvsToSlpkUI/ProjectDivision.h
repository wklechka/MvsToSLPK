#pragma once
#include <string>
#include <vector>

void quadTreeTest();

namespace ProjectDivision
{
	struct ProjectDiv
	{
		// Inputs
		std::wstring filename; 
		std::wstring workingFolder;
		int maxImages = 100;
		int minImages = 10;
		double overlapPercentage = 0.2;
		bool showQuadtree = false;
		// output
		std::vector<std::string> projectsGenerated;
	};

	bool divideProject(ProjectDiv &opt);
};

