#include "pch.h"
#include "CoordSys.h"
#include "UtilityFunc.h"

bool SetSpatialRefFromProj(OGRSpatialReference& sr, std::string projection, bool IsESRIformat = false);
OGRCoordinateTransformation* getTransform(const std::string& projFile);

bool SetSpatialRefFromProj(OGRSpatialReference& sr, std::string projection, bool IsESRIformat)
{
	char* ptrToExistingDTM = const_cast<char*>(projection.c_str());
	if (IsESRIformat)
	{
		if (OGRERR_NONE != sr.importFromESRI(&ptrToExistingDTM))
			return false;
	}
	else
		if (OGRERR_NONE != sr.importFromWkt(&ptrToExistingDTM))
			return false;
	return true;
}

// GDAL https://gdal.org/tutorials/osr_api_tut.html
void getWGS84LatLon(OGRSpatialReference& geoSR)
{
	// Define geographic spatial reference
	//OGRSpatialReference geoSR;
// 	geoSR.SetGeogCS("Geographic coordinate system",
// 		"WGS_1984",
// 		"My Spheroid",
// 		6378137, 298.257223563,
// 		SRS_PM_GREENWICH, 0.0,
// 		"degree", M_PI / 180.0); // SRS_UA_DEGREE_CONV = pi/180

	const double degConv = atof(SRS_UA_DEGREE_CONV);
	geoSR.SetGeogCS("Geographic coordinate system",
		"World Geodetic System 1984",
		"My WGS84 Spheroid",
		SRS_WGS84_SEMIMAJOR, SRS_WGS84_INVFLATTENING,
		SRS_PM_GREENWICH, 0.0,
		SRS_UA_DEGREE, degConv);

	geoSR.SetVertCS("WGS84", "WGS84", 2005); // Set vertical units to meters
}

OGRCoordinateTransformation* getTransform(const std::string& projFile)
{
	slpkUtilityFunc::setupProjDB();

	std::string prj_file_name = projFile;
	std::string projection = slpkUtilityFunc::GetProjectionWKTSTring(prj_file_name);

	OGRSpatialReference src;
	SetSpatialRefFromProj(src, projection);
	OGRSpatialReference dest;
	getWGS84LatLon(dest);

	OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&src, &dest);

	return poCT;
}

static OGRCoordinateTransformation* getTransformFromStr(const std::string& projection)
{
	slpkUtilityFunc::setupProjDB();

	OGRSpatialReference src;
	SetSpatialRefFromProj(src, projection);
	OGRSpatialReference dest;
	getWGS84LatLon(dest);

	OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&src, &dest);

	return poCT;
}

static OGRCoordinateTransformation* getTransformFromStrECEF(const std::string& projection)
{
	slpkUtilityFunc::setupProjDB();

	OGRSpatialReference src;
	SetSpatialRefFromProj(src, projection);
	OGRSpatialReference dest;
	dest.SetGeocCS("WGS 84");

	OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&src, &dest);

	return poCT;
}

void CoordConverter::cleanup()
{
	if (m_poCT) {
		OGRCoordinateTransformation::DestroyCT(m_poCT);
	}
	if (m_ECEF) {
		OGRCoordinateTransformation::DestroyCT(m_ECEF);
	}
}
CoordConverter::~CoordConverter()
{
	cleanup();
}

void CoordConverter::setFromFile(const std::string& projFile)
{
	setString(slpkUtilityFunc::GetProjectionWKTSTring(projFile));
}
void CoordConverter::setString(const std::string& projection)
{
	cleanup();

	m_prjStr = projection;

	OGRSpatialReference src;
	SetSpatialRefFromProj(src, projection);
	m_dUnit = src.GetLinearUnits();

	m_poCT = getTransformFromStr(m_prjStr);

	m_firstCheck = true;

	// make the ECEF version
	m_ECEF = getTransformFromStrECEF(m_prjStr);
}

bool CoordConverter::transform(double& x, double& y, double& z)
{

	if (m_firstCheck) {
		m_convertZ = false;
		if (m_dUnit != 1.0) {
			double testX = x;
			double testY = y;
			double testZ = z;
			double lastZ = z;
			m_poCT->Transform(1, &testX, &testY, &testZ);
			if (lastZ == testZ) {
				// z not adjusted by the transform
				m_convertZ = true;
			}
		}

		m_firstCheck = false;
	}
	
	m_poCT->Transform(1, &x, &y, &z);
	
	std::swap(x, y);
	if (m_convertZ) {
		z *= m_dUnit;
	}

	return true;
}

bool CoordConverter::transformECEF(double& x, double& y, double& z)
{
	// just going to assume z need to be meters
	// cannot use the Z test like the Transform() func because ECEF always changes Z
	z *= m_dUnit;

	m_ECEF->Transform(1, &x, &y, &z);
	return true;
}