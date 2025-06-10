#include "pch.h"
#include "IImageReader.h"


#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "gdal.h"
#include "gdal_priv.h"
#include <fstream>
#include <stack>
#include <algorithm>
#include <string>
#include <mutex>
#include "StdUtil/StdUtility.h"
#include "StdUtil/WinUtility.h"
#include "gdalCommon.h"


class IGDALReaderImpl : public IImageReader
{
public:
	IGDALReaderImpl();
	~IGDALReaderImpl();

	virtual void Release() override;

	virtual void ignoreUpperBands(bool val) override { _ignoreUpperBands = val; }

	virtual bool setFile(const char* filename) override;
	virtual bool getImageInfo(GDALImageInfo& info) override;

	virtual bool getImageInfo(const char* filename, GDALImageInfo& info) override;
	virtual bool getGeoInfo(IImageFormat_GeoInfo& geoInfo) override;

	virtual unsigned int readRectMemSize(int nXSize, int nYSize) override;
	virtual bool readRect(int nXOff, int nYOff, int nXSize, int nYSize, BYTE* data) override;
	virtual bool readRect(int nXOff, int nYOff, int nXSize, int nYSize, int nBufXSize, int nBufYSize, BYTE* data) override;

	virtual bool setImageNumber(int num) override;

protected:
	std::string _filename;
	GDALImageInfo _info;
	int _imageNum = 0;

	void EPSGcodes(char* ptrPretty, int& model, int& sys, int& datum, int& unitCode);

	GDALDataset* _poDataset = nullptr;
	void cleanup();

	bool _ignoreUpperBands = false;
};

IIMAGEREADER_DLL IImageReader* createIImageReader()
{
	initGDAL();

	IGDALReaderImpl* impl = new IGDALReaderImpl;

	return impl;
}

IGDALReaderImpl::IGDALReaderImpl()
{
}

IGDALReaderImpl::~IGDALReaderImpl()
{
	cleanup();
}

void IGDALReaderImpl::cleanup()
{
	if (_poDataset)
	{
		GDALClose(_poDataset);
	}
	_poDataset = nullptr;
}

void IGDALReaderImpl::Release()
{
	cleanup();
	delete this;
}

bool IGDALReaderImpl::setFile(const char* filename)
{
	_filename = filename;

	cleanup();

	_poDataset = GDALDataset::FromHandle(GDALOpen(_filename.data(), GA_ReadOnly));

	if (_poDataset == nullptr)
	{
		return false;
	}

	return getImageInfo(filename, _info);
}

bool IGDALReaderImpl::getImageInfo(GDALImageInfo& info)
{
	info = _info;
	return true;
}

class Point3D
{
public:
	double x_, y_, z_;
};

static Point3D matMult(const Point3D& vec, double mat[4][4])
{
	Point3D newVec;

	double fInvW = 1.0 / (mat[0][3] * vec.x_ + mat[1][3] * vec.y_ + mat[2][3] * vec.z_ + mat[3][3]);

	newVec.x_ = (mat[0][0] * vec.x_ + mat[1][0] * vec.y_ + mat[2][0] * vec.z_ + mat[3][0]) * fInvW;
	newVec.y_ = (mat[0][1] * vec.x_ + mat[1][1] * vec.y_ + mat[2][1] * vec.z_ + mat[3][1]) * fInvW;
	newVec.z_ = (mat[0][2] * vec.x_ + mat[1][2] * vec.y_ + mat[2][2] * vec.z_ + mat[3][2]) * fInvW;

	return newVec;
}

bool IGDALReaderImpl::getGeoInfo(IImageFormat_GeoInfo& geoInfo)
{
	if (!_info._hasGeoInfo)
		return false;

	// Cast to a 4x4 matrix
	auto (*matrix)[4] = reinterpret_cast<double(*)[4]>(geoInfo._matrix);

	matrix[3][0] = _info._adfGeoTransform[0];
	matrix[0][0] = _info._adfGeoTransform[1];
	matrix[0][1] = _info._adfGeoTransform[2];
	matrix[3][1] = _info._adfGeoTransform[3];
	matrix[1][0] = _info._adfGeoTransform[4];
	matrix[1][1] = _info._adfGeoTransform[5];

	// no longer adjusted
	Point3D vec = { 0.0, 0.0, 0.0 };
	Point3D newVec = matMult(vec, matrix);

	geoInfo._upperLeftGeo[0] = newVec.x_;
	geoInfo._upperLeftGeo[1] = newVec.y_;

	// fix to match us
	matrix[3][0] = geoInfo._upperLeftGeo[0];
	matrix[3][1] = geoInfo._upperLeftGeo[1];

	Point3D vec2 = { _info._imageWidth - 1, _info._imageHeight - 1, 0.0 };
	newVec = matMult(vec2, matrix);

	geoInfo._lowerRightGeo[0] = newVec.x_;
	geoInfo._lowerRightGeo[1] = newVec.y_;


	geoInfo._scaleX = matrix[0][0];
	geoInfo._rotX = matrix[0][1];

	geoInfo._rotY = matrix[1][0];
	geoInfo._scaleY = matrix[1][1];

	geoInfo._sys = _info._sys;
	geoInfo._modelType = _info._model;
	geoInfo._datum = _info._datum;
	geoInfo._unitCode = _info._unitCode;

	geoInfo._pixelType = IImageFormat_GeoInfo::CENTER;
	if (_info._wkt[0] != '\0') {
		geoInfo._wkt[0] = '\0';
		strncpy_s(geoInfo._wkt, _info._wkt, 2047);
	}

	return true;
}

inline void rtrimZeros(std::string& s, unsigned char valToTrim) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [valToTrim](unsigned char ch) {
		return valToTrim != ch;
		}).base(), s.end());

	if (s.size() > 0) {
		if (s[s.size() - 1] == '.') {
			s.push_back(valToTrim);
		}
	}
}
std::string getAsString(const char* val)
{
	if (val)
	{
		return std::string(val);
	}
	return std::string();
}

bool IGDALReaderImpl::getImageInfo(const char* filename, GDALImageInfo& info)
{
	GDALDataset* poDataset = GDALDataset::FromHandle(GDALOpen(filename, GA_ReadOnly));
	if (poDataset == NULL)
	{
		return false;
	}

	// fill in the info
	if (poDataset->GetGeoTransform(info._adfGeoTransform) == CE_None)
	{
		info._hasGeoInfo = true;
	}
	else {
		info._hasGeoInfo = false;
	}

	const OGRSpatialReference* sr = poDataset->GetSpatialRef();

	info._wkt[0] = '\0';
	if (sr) {
		//	const char *projRef = poDataset->GetProjectionRef();
		char* ptrPretty;
		sr->exportToPrettyWkt(&ptrPretty);
		// 	std::ofstream	file;
		// 	file.open("F:\\tmp\\outSys.txt");
		// 	file << ptrPretty << std::endl;
		// 	file.close();

		strncpy_s(info._wkt, ptrPretty, 2048);

		EPSGcodes(ptrPretty, info._model, info._sys, info._datum, info._unitCode);
		CPLFree(ptrPretty);
	}

	info._isJpeg = false;
	info._desc[0] = '\0';
	info._compressionMeth[0] = '\0';
	info._longName[0] = '\0';
	info._software[0] = '\0';
	info._maxSampleVal = 0;

	if (poDataset->GetDriver()) {
		std::string descript = poDataset->GetDriver()->GetDescription();
		strncpy_s(info._desc, descript.c_str(), IGDAL_MAX_STRING);
		info._isJpeg = strcmp(info._desc, "JPEG") == 0;

		std::string longName = getAsString(poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
		strcat_s(info._longName, longName.c_str());
	}
	else {
		return false;
	}

	auto dataInList = [](char** dict)
		{
			std::string test;
			int numInList = CSLCount(dict);
			for (int i = 0; i < numInList; ++i) {
				const char* item = CSLGetField(dict, i);
				test += item;
				test += " ";
			}
			return test;
		};

	const char* softwareTag = poDataset->GetMetadataItem("TIFFTAG_SOFTWARE");
	if (softwareTag) {
		strcat_s(info._software, softwareTag);
	}

	const char* maxSampleTag = poDataset->GetMetadataItem("TIFFTAG_MAXSAMPLEVALUE");
	if (maxSampleTag) {
		info._maxSampleVal = atoi(maxSampleTag);
	}

	char** dict = poDataset->GetMetadata("IMAGE_STRUCTURE");
	const char* compStr = CSLFetchNameValue(dict, "COMPRESSION");


	info._hasEXIF = false;
	const char* focalLength = CSLFetchNameValue(poDataset->GetMetadata(), "EXIF_FocalLength");
	if (focalLength) {
		info._hasEXIF = true;
	}

	if (compStr) {
		strcat_s(info._compressionMeth, compStr);
	}
	else {
		strcat_s(info._compressionMeth, "None");
	}

	std::string compressionStr = info._compressionMeth;
	if (compressionStr == "JXL") {
		const char* effort = CSLFetchNameValue(dict, "JXL_EFFORT");
		const char* lossless = CSLFetchNameValue(dict, "COMPRESSION_REVERSIBILITY");
		const char* jxlDistance = CSLFetchNameValue(dict, "JXL_DISTANCE");


		std::string lossLessStr;
		if (lossless) {
			lossLessStr = lossless;
			if (lossLessStr == "LOSSLESS") {
				strcat_s(info._compressionMeth, " ");
				strcat_s(info._compressionMeth, "Lossless");
			}
		}
		if (lossLessStr != "LOSSLESS") {
			if (effort) {
				std::string effortStr = effort;
				strcat_s(info._compressionMeth, " effort:");
				strcat_s(info._compressionMeth, effortStr.c_str());
			}
			if (jxlDistance) {
				std::string distStr = jxlDistance;
				strcat_s(info._compressionMeth, " Dist:");
				rtrimZeros(distStr, '0');
				strcat_s(info._compressionMeth, distStr.c_str());
			}
		}
	}

	info._isCOG = false;
	const char* layout = CSLFetchNameValue(dict, "LAYOUT");
	if (layout) {
		std::string lay = layout;
		if (lay == "COG") {
			strcat_s(info._longName, ", COG");
			info._isCOG = true;
		}
	}


	info._imageWidth = poDataset->GetRasterXSize();
	info._imageHeight = poDataset->GetRasterYSize();
	info._numChannels = poDataset->GetRasterCount();

	if (_ignoreUpperBands) {
		if (info._numChannels > 3) {
			info._numChannels = 3;
		}
	}

	GDALRasterBand* poBand;
	int             nBlockXSize, nBlockYSize;
	poBand = poDataset->GetRasterBand(1);

	if (!poBand) {
		// no bands
		GDALClose(poDataset);
		return false;
	}


	//info._numImages = poBand->GetOverviewCount() + 1;
	// calculate virtual or maybe real images
	info._numImages = 1;

	int maxDim = min(info._imageWidth, info._imageHeight);
	// figure out how many virtual levels
	while (maxDim > 64) {
		maxDim /= 2;
		++_info._numImages;
	}


	poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);

	// 	int test1, test2;
	// 	poBand->GetActualBlockSize(0, 0, &test1, &test2);
	// 	double test3 = poBand->GetMaximum();

	info._tileWidth = nBlockXSize;
	info._tileHeight = nBlockYSize;

	// cant seem to get the actual bitsPerSample
	
	switch (poBand->GetRasterDataType()) {
	case GDT_Byte:
		info._bytesPerChannel = 1;
		info._bitsPerSample = 8;
		break;
	default:
		info._bytesPerChannel = 2;
		info._bitsPerSample = 16;
		break;
	}

	info._colorInterp = poBand->GetColorInterpretation();
	info._overviewCount = poBand->GetOverviewCount();

	double min = poBand->GetMinimum();
	double max = poBand->GetMaximum();

	info._bytesPerPixel = info._numChannels * info._bytesPerChannel;

	// rpc info
	char** dictRPC = poDataset->GetMetadata("RPC");
	if (dictRPC) {

		auto setRPCVal = [](char** dictRPC, const char* VAL) -> double
			{
				if (!dictRPC) {
					return 0.0;
				}

				const char* compStr1 = CSLFetchNameValue(dictRPC, VAL);
				if (compStr1) {
					return atof(compStr1);
				}
				return 0.0;
			};

		info.hasRPCData = true;
		info._rpcData._lineOffset = setRPCVal(dictRPC, "LINE_OFF");
		info._rpcData._sampOffset = setRPCVal(dictRPC, "SAMP_OFF");
		info._rpcData._latOffset = setRPCVal(dictRPC, "LAT_OFF");
		info._rpcData._longOffset = setRPCVal(dictRPC, "LONG_OFF");
		info._rpcData._heightOffset = setRPCVal(dictRPC, "HEIGHT_OFF");

		info._rpcData._lineScale = setRPCVal(dictRPC, "LINE_SCALE");
		info._rpcData._sampScale = setRPCVal(dictRPC, "SAMP_SCALE");
		info._rpcData._latScale = setRPCVal(dictRPC, "LAT_SCALE");
		info._rpcData._longScale = setRPCVal(dictRPC, "LONG_SCALE");
		info._rpcData._heightScale = setRPCVal(dictRPC, "HEIGHT_SCALE");

		auto setRPCValCOEFF = [](char** dictRPC, const char* VAL, double* dblVals)
			{
				if (!dictRPC) {
					return;
				}

				const char* compStr1 = CSLFetchNameValue(dictRPC, VAL);
				if (compStr1) {
					std::vector<std::string> words;
					std::string line = compStr1;

					StdUtility::tokenizeLine(line, " ", words);

					for (int i = 0; i < words.size() && i < 20; ++i) {
						dblVals[i] = atof(words[i].c_str());
					}
				}
				return;
			};

		setRPCValCOEFF(dictRPC, "LINE_NUM_COEFF", info._rpcData._lineNumCoef);
		setRPCValCOEFF(dictRPC, "LINE_DEN_COEFF", info._rpcData._lineDenCoef);
		setRPCValCOEFF(dictRPC, "SAMP_NUM_COEFF", info._rpcData._sampNumCoef);
		setRPCValCOEFF(dictRPC, "SAMP_DEN_COEFF", info._rpcData._sampDenCoef);
	}
	else {
		info.hasRPCData = false;
		info._rpcData._lineOffset = 0.0;
		info._rpcData._sampOffset = 0.0;
		info._rpcData._latOffset = 0.0;
		info._rpcData._longOffset = 0.0;
		info._rpcData._heightOffset = 0.0;

		info._rpcData._lineScale = 0.0;
		info._rpcData._sampScale = 0.0;
		info._rpcData._latScale = 0.0;
		info._rpcData._longScale = 0.0;
		info._rpcData._heightScale = 0.0;
		for (int i = 0; i < 20; ++i) {
			info._rpcData._lineNumCoef[i] = 0.0;
			info._rpcData._lineDenCoef[i] = 0.0;
			info._rpcData._sampNumCoef[i] = 0.0;
			info._rpcData._sampDenCoef[i] = 0.0;
		}
	}

	GDALClose(poDataset);

	return true;
}

unsigned int IGDALReaderImpl::readRectMemSize(int nXSize, int nYSize)
{
	return nXSize * _info._bytesPerPixel * nYSize;
}

bool IGDALReaderImpl::readRect(int nXOff, int nYOff, int nBufXSize, int nBufYSize, BYTE* data)
{
	int nXSize = nBufXSize;
	int nYSize = nBufYSize;

	if (_imageNum > 0) {
		int mult = pow(2, _imageNum);

		// find offset on 1x
		nXOff *= mult;
		nYOff *= mult;

		// size on 1x
		nXSize *= mult;
		nYSize *= mult;
	}

	return readRect(nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize, data);
}

bool IGDALReaderImpl::readRect(int nXOff, int nYOff, int nXSize, int nYSize, int nBufXSize, int nBufYSize, BYTE* data)
{

	if (!_poDataset)
		return false; // setFile never called

	GDALDataType dataType = GDT_Byte;

	// only doing word and byte data
	if (_info._bytesPerChannel == 2) {
		dataType = GDT_UInt16;
	}

	// make some mem for our bands
	std::vector<void*> bandMem;

	for (int i = 0; i < _info._numChannels; ++i) {
		GDALRasterBand* poBand;
		poBand = _poDataset->GetRasterBand(i + 1);

		bandMem.push_back(nullptr);

		if (!poBand)
			break;

		// make some mem
		bandMem[bandMem.size() - 1] = CPLMalloc(_info._bytesPerChannel * nBufXSize * nBufYSize);

		GDALRasterIOExtraArg sExtraArg;
		INIT_RASTERIO_EXTRA_ARG(sExtraArg);
		sExtraArg.eResampleAlg = GRIORA_Cubic;


		CPLErr err = poBand->RasterIO(GF_Read, nXOff, nYOff, nXSize, nYSize,
			bandMem[bandMem.size() - 1], nBufXSize, nBufYSize, dataType,
			0, 0, &sExtraArg);
	}

	// put the data into the passed in buffer
	if (_info._bytesPerChannel == 2) {
		WORD* destPtr = (WORD*)data;
		for (int j = 0; j < nBufYSize; ++j) {
			for (int k = 0; k < nBufXSize; ++k) {
				for (int i = 0; i < _info._numChannels; ++i) {
					*(destPtr++) = *((WORD*)(bandMem[i]) + k + (j * nBufXSize));
				}
			}
		}
	}
	else {
		BYTE* destPtr = data;
		for (int j = 0; j < nBufYSize; ++j) {
			for (int k = 0; k < nBufXSize; ++k) {
				for (int i = 0; i < _info._numChannels; ++i) {
					*(destPtr++) = *((BYTE*)(bandMem[i]) + k + (j * nBufXSize));
				}
			}
		}
	}

	for (int i = 0; i < _info._numChannels; ++i) {
		CPLFree(bandMem[i]);
	}


	return true;
}

bool IGDALReaderImpl::setImageNumber(int num)
{
	if (num < 0) {
		_imageNum = 0;
	}
	_imageNum = num;

	getImageInfo(_filename.data(), _info);

	_info._imageWidth /= pow(2.0, num);
	_info._imageHeight /= pow(2.0, num);

	return true;
}

class wktElement
{
public:
	std::string _name;

	std::vector<wktElement*> _children;
	std::vector<std::string> _values; // could be strings or numbers
};



static wktElement* findElement(wktElement* root, const std::string& elementName)
{
	// transverse to find element
	if (root->_name == elementName) {
		// element is found
		return root;
	}
	else {
		for (int i = 0; i < root->_children.size(); ++i) {
			wktElement* elm = findElement(root->_children[i], elementName);
			if (elm) return elm;
		}
	}

	return nullptr;
}

static int findEPSGcode(wktElement* root, const std::string& elementName)
{
	wktElement* elem = findElement(root, elementName);

	for (auto iter : elem->_children) {
		if (iter->_name == "AUTHORITY") {
			if (iter->_values.size() > 1) {
				return atoi(iter->_values[1].c_str());
			}
		}
	}

	return 0;
}

static int findEPSGcode(wktElement* root, const std::string& parentName, const std::string& childName)
{
	wktElement* elem = findElement(root, parentName);

	//now find the child
	for (auto iter : elem->_children) {
		if (iter->_name == childName) {
			for (auto iterChild : iter->_children) {
				if (iterChild->_name == "AUTHORITY") {
					if (iterChild->_values.size() > 1) {
						return atoi(iterChild->_values[1].c_str());
					}
				}
			}
			break;
		}
	}

	return 0;
}

static void clearTree(wktElement* root)
{
	for (int i = 0; i < root->_children.size(); ++i) {
		clearTree(root->_children[i]);
	}
	delete root;
}

static std::string removeCharFromString(const std::string& str, char ch) {
	std::string result = str;
	result.erase(std::remove(result.begin(), result.end(), ch), result.end());
	return result;
}


static std::string cleanStr(std::string& str)
{
	std::string tmpStr = str;
	StdUtility::trim(tmpStr);
	removeCharFromString(tmpStr, '\"');
	return tmpStr;
}

void IGDALReaderImpl::EPSGcodes(char* ptrPretty, int& model, int& sys, int& datum, int& unitCode)
{
	model = 0;
	sys = 0;
	datum = 0;
	unitCode = 0;

	std::string currentStr;

	wktElement* rootElm = nullptr;
	char* charPtr = ptrPretty;
	std::stack<wktElement*> nodeStack;

	// get a root node
	while (*charPtr != '\0') {
		if (*charPtr == '[') {
			wktElement* newElem = new wktElement;
			newElem->_name = currentStr;
			currentStr = "";
			rootElm = newElem;
			++charPtr;
			break;
		}
		else {
			currentStr += *charPtr;
		}
		++charPtr;
	}

	if (!rootElm) return;

	// push the root on the stack
	nodeStack.push(rootElm);

	wktElement* currentElm = rootElm;
	while (!nodeStack.empty() && *charPtr != '\0') {

		wktElement* currentElm = nodeStack.top();

		if (*charPtr == '[') {
			wktElement* newElem = new wktElement;
			newElem->_name = cleanStr(currentStr);
			currentStr = "";
			currentElm->_children.push_back(newElem);
			nodeStack.push(newElem);
		}
		else if (*charPtr == ']') {
			// the end for the current node
			nodeStack.pop();
			if (!currentStr.empty()) {
				currentElm->_values.push_back(cleanStr(currentStr));
				currentStr = "";
			}
		}
		else if (*charPtr == ',') {
			if (!currentStr.empty()) {
				currentElm->_values.push_back(cleanStr(currentStr));
				currentStr = "";
			}
		}
		else {
			currentStr += *charPtr;
		}
		++charPtr;
	}

	// find values in tree
	wktElement* elmCS = findElement(rootElm, std::string("PROJCS"));
	if (elmCS) {
		model = 1;
		sys = findEPSGcode(rootElm, std::string("PROJCS"));
		unitCode = findEPSGcode(rootElm, std::string("PROJCS"), std::string("UNIT"));
		datum = findEPSGcode(rootElm, std::string("GEOGCS"), std::string("DATUM"));
	}

	wktElement* elmGeo = findElement(rootElm, std::string("GEOCCS"));
	if (elmGeo) {
		model = 2;
		sys = findEPSGcode(rootElm, std::string("GEOCCS"));
		unitCode = findEPSGcode(rootElm, std::string("GEOCCS"), std::string("UNIT"));
		datum = findEPSGcode(rootElm, std::string("GEOGCS"), std::string("DATUM"));
	}

	clearTree(rootElm);
}