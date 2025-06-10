#pragma once
#include <string>

namespace slpkUtilityFunc
{
	bool boxFile(const std::string& boxFileName, double boxData[9]);
	std::string GetProjectionWKTSTring(const std::string& prjFileName);
	void setupProjDB();
};

