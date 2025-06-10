#pragma once
#include <string>
#include "gdal.h"
#include "ogr_core.h"
#include "ogr_spatialref.h"

class CoordConverter
{
public:
	~CoordConverter();

	void setFromFile(const std::string& projFile);
	void setString(const std::string& projection);

	// transforms to Lat/lon
	bool transform(double& x, double& y, double& z);

	// transforms to ECEF
	bool transformECEF(double& x, double& y, double& z);

protected:
	std::string m_prjStr;
	double m_dUnit = 1.0;
	OGRCoordinateTransformation* m_poCT = nullptr;
	bool m_convertZ = false;
	bool m_firstCheck = true;

	// defaults to output WGS84 lat long
	bool m_outECEF = false;
	OGRCoordinateTransformation* m_ECEF = nullptr;

	void cleanup();
};
