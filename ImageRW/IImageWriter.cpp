#include "pch.h"
#include "IImageWriter.h"

#include "cpl_conv.h"
#include "cpl_vsi.h"
#include "gdal.h"
#include "gdal_priv.h"
#include <fstream>
#include <stack>
#include <algorithm>

#include "StdUtil/StdStringUtil.h"

#include "gdalCommon.h"

class IWriteGDALImpl : public IImageWriter
{
public:
	IWriteGDALImpl();
	~IWriteGDALImpl();

	virtual void Release() override;

	virtual void setProgressCallback(IWriteGDALProgressCallback callback, void* clientData) override;
	virtual void cancelEvent(void* event) { }

	virtual bool start(const char* filename, const GDALWriteOptions& opt, const char* copyGeoInfoFromFile, const char* driverName) override;
	virtual bool writeTile(const char* tileData) override;
	virtual bool finish(bool wasGood) override;

	virtual int tilesPerImage() override;

	virtual bool createCopy(const char* srcFilename, const char* dstFilename,
		GDALCreateCopyOptions opt, const char* driverName) override;
	virtual bool createCopy(const char* srcFilename, const char* dstFilename,
		char** papszOptions, const char* driverName) override;

	virtual bool writeAsTiff(uint8_t* bits, int width, int height, const char* filename, int bytesPerPixel);

protected:
	GDALWriteOptions _opt;
	GDALDatasetH hOutDS = NULL;

	std::string _filename;

	uint32_t _tilesAcross;
	uint32_t _tilesDown;
	uint32_t _tilesPerImage;
	uint32_t _writeTileSize;
	uint32_t _tileNum;
	uint32_t _bytesPerPixel;

	GDALDataType _dataType = GDT_Byte;

	IWriteGDALProgressCallback _callback = nullptr;
	void* _clientData = nullptr;

	void genZoomLevels();

	bool* _cancelEvent = nullptr;
};

struct ProgressDataForGdal
{
	IWriteGDALProgressCallback _callback = nullptr;
	void* _clientData = nullptr;

	bool userCanceled()
	{
// 		if (_cancelEvent) {
// 			if (::WaitForSingleObject(_cancelEvent->operator HANDLE(), 0) != WAIT_TIMEOUT) {
// 				return true;
// 			}
// 		}
		return false;
	}
};

IImageWriter* createIImageWriter()
{
	initGDAL();

	IWriteGDALImpl* impl = new IWriteGDALImpl;
	return impl;
}

IWriteGDALImpl::IWriteGDALImpl()
{

}
IWriteGDALImpl::~IWriteGDALImpl()
{

}

void IWriteGDALImpl::Release()
{
	delete this;
}

void IWriteGDALImpl::setProgressCallback(IWriteGDALProgressCallback callback, void* clientData)
{
	_callback = callback;
	_clientData = clientData;
}

int CPL_STDCALL GDALProgressCallback(double dfComplete,
	CPL_UNUSED const char* pszMessage,
	CPL_UNUSED void* pProgressArg)
{
	if (!pProgressArg)
		return TRUE;

	ProgressDataForGdal* progressData = (ProgressDataForGdal*)pProgressArg;

	if (progressData->userCanceled())
		return FALSE;

	if (progressData->_callback) {
		IWriteGDALProgress prog;
		prog.start = 0;
		prog.stop = 100;
		prog.progress = int(100.0 * dfComplete);
		//prog.progressStr = pszMessage;
		progressData->_callback(prog, progressData->_clientData);
	}

	return TRUE;
}

bool IWriteGDALImpl::start(const char* filename, const GDALWriteOptions& opt, const char* copyGeoInfoFromFile, const char* driverName)
{
	_tileNum = 0;
	_opt = opt;
	_filename = filename;

	_dataType = _opt.bytesPerChannel == 1 ? GDT_Byte : GDT_UInt16;

	if (_opt.makeDTMFormat) {
		_opt.bytesPerChannel = 4;
		_opt.numChannels = 1;
		_dataType = GDT_Float32;
	}

	_tilesAcross = (_opt.width + _opt.tileWidth - 1) / _opt.tileWidth;
	_tilesDown = (_opt.height + _opt.tileHeight - 1) / _opt.tileHeight;
	_tilesPerImage = _tilesAcross * _tilesDown;
	_writeTileSize = _opt.tileWidth * _opt.tileHeight * _opt.numChannels * _opt.bytesPerChannel;
	_tileNum = 0;
	_bytesPerPixel = _opt.numChannels * _opt.bytesPerChannel;


	GDALDriverH hDriver = GDALGetDriverByName(driverName ? driverName : "GTiff");
	//GDALDriverH hDriver = GDALGetDriverByName(driverName ? driverName : "MRF");

	//std::string canCreate = getAsString(poDataset->GetDriver()->GetMetadataItem(GDAL_DCAP_CREATE));
	//std::string canCreateCopy = getAsString(poDataset->GetDriver()->GetMetadataItem(GDAL_DCAP_CREATECOPY));


	std::string tileWidthStr = StdUtility::string_format("%d", _opt.tileWidth);
	std::string tileHeightStr = StdUtility::string_format("%d", _opt.tileWidth);

	char** papszOptions = NULL;
	if (_opt.tileWidth > 0 && _opt.tileHeight > 0) {
		papszOptions = CSLSetNameValue(papszOptions, "TILED", "YES");
		papszOptions = CSLSetNameValue(papszOptions, "BLOCKXSIZE", tileWidthStr.c_str());
		papszOptions = CSLSetNameValue(papszOptions, "BLOCKYSIZE", tileHeightStr.c_str());
	}

	if (_opt.makeDTMFormat) {
		//papszOptions = CSLSetNameValue(papszOptions, "TIFFTAG_GDAL_NODATA ASCII", "-9999");
		papszOptions = CSLSetNameValue(papszOptions, "TIFFTAG_GDAL_NODATA", "-9999");
	}

	if (_opt.numChannels == 1) {
		papszOptions = CSLSetNameValue(papszOptions, "PHOTOMETRIC", "MINISBLACK");
	}
	else {
		papszOptions = CSLSetNameValue(papszOptions, "PHOTOMETRIC", "RGB");
	}

	//= [BAND, PIXEL]
	papszOptions = CSLSetNameValue(papszOptions, "INTERLEAVE", "PIXEL");

	// GDAL will create real 9-15 images not just 16 bit with a different maxsamplevalue
	// We have always written 12 or 16 bit imagery as 16 with a maxsample value set
	// so we will always set to 16

	std::string tmp = StdUtility::string_format("%d", _opt.bitsPerSample);
	papszOptions = CSLSetNameValue(papszOptions, "NBITS", tmp.c_str());

	int maxSampleValue = _opt.maxSampleValue;
	if (maxSampleValue == 0) {
		maxSampleValue = int(pow(2.0, opt.bitsPerSample) - 1);
	}

	if (_opt.bytesPerChannel > 1) {
		if (opt.bitsPerSample > 8 && opt.bitsPerSample < 16) {
			// must us full 16 with JXL
			std::string tmp = StdUtility::string_format("%d", 16);
			papszOptions = CSLSetNameValue(papszOptions, "NBITS", tmp.c_str());
		}
	}

	if (_opt._compType != GDALWriteOptions::GW_NONE) {
		switch (_opt._compType) {
		case GDALWriteOptions::GW_JPEG:
			papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "JPEG");
			papszOptions = CSLSetNameValue(papszOptions, "JPEG_QUALITY", "75");
			break;
		case GDALWriteOptions::GW_JXL:
		{
			papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "JXL");
			papszOptions = CSLSetNameValue(papszOptions, "JXL_LOSSLESS", _opt.jxlLossless ? "YES" : "NO");
			// the amount of compression 1-9, The higher, the smaller file
			std::string amt = StdUtility::string_format("%d", _opt.jxlEffort);
			papszOptions = CSLSetNameValue(papszOptions, "JXL_EFFORT", amt.c_str());

			papszOptions = CSLSetNameValue(papszOptions, "INTERLEAVE", "BAND");
		}
		break;
		case GDALWriteOptions::GW_LERC:
			papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "LERC");
			break;
		case GDALWriteOptions::GW_LZW:
			papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "LZW");
			break;

		case GDALWriteOptions::GW_NONE:

			break;
		}
	}

	// our default is to only create big tiff if it is asked for...
	papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", _opt._createBigTiff ? "YES" : "NO");
	if (_opt.makeDTMFormat) {
		// This option automatically makes bigTiff if needed...
		papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", "IF_NEEDED");
	}

	CPLSetConfigOption("GTIFF_POINT_GEO_IGNORE", "TRUE");

	hOutDS = GDALCreate(hDriver, filename,
		_opt.width, _opt.height, _opt.numChannels, _dataType,
		papszOptions);

	CSLDestroy(papszOptions);

	if (hOutDS == NULL)
		return false;

	GDALDataset* dataSetOut = GDALDataset::FromHandle(hOutDS);

	//dataSetOut->SetMetadataItem("TIFFTAG_SOFTWARE", "TAG TEST");
	if (!_opt.makeDTMFormat) {
		std::string maxValStr = StdUtility::string_format("%d", maxSampleValue);
		dataSetOut->SetMetadataItem("TIFFTAG_MAXSAMPLEVALUE", maxValStr.c_str());
	}

	if (_opt.hasGeoInfo) {

		dataSetOut->SetMetadataItem(GDALMD_AREA_OR_POINT, GDALMD_AOP_POINT);

		double adfGeoTransform[6];
		double mAsGrid[4][4];
		memcpy(mAsGrid, _opt.geoInfo._matrix, sizeof(mAsGrid));

		adfGeoTransform[0] = mAsGrid[3][0];
		adfGeoTransform[1] = mAsGrid[0][0];
		adfGeoTransform[2] = mAsGrid[0][1];
		adfGeoTransform[3] = mAsGrid[3][1];
		adfGeoTransform[4] = mAsGrid[1][0];
		adfGeoTransform[5] = mAsGrid[1][1];

		GDALSetGeoTransform(hOutDS, adfGeoTransform);
	}

	if (copyGeoInfoFromFile) {
		double adfGeoTransform[6];
		// set more geoinfo
		GDALDatasetH hDataset = GDALOpen(copyGeoInfoFromFile, GA_ReadOnly);
		GDALDataset* dataSetIn = GDALDataset::FromHandle(hDataset);

		if (hDataset) {
			GDALDataset* dataSetOut = GDALDataset::FromHandle(hOutDS);
			// GDALMD_AOP_POINT
			const char* pixelType = dataSetIn->GetMetadataItem(GDALMD_AREA_OR_POINT);

			if (pixelType) {
				dataSetOut->SetMetadataItem(GDALMD_AREA_OR_POINT, pixelType);
			}
			else {
				dataSetOut->SetMetadataItem(GDALMD_AREA_OR_POINT, GDALMD_AOP_POINT);
			}

			// get geo info
			const char* projection = dataSetIn->GetProjectionRef();
			const OGRSpatialReference* spat = dataSetIn->GetSpatialRef();
			dataSetIn->GetGeoTransform(adfGeoTransform);

			// set geoInfo
			dataSetOut->SetGCPs(GDALGetGCPCount(hDataset), GDALGetGCPs(hDataset), spat);
			dataSetOut->SetProjection(projection);
			dataSetOut->SetSpatialRef(spat);
			dataSetOut->SetGeoTransform(adfGeoTransform);

			//dataSet->SetMetadataItem(GDALMD_AREA_OR_POINT, GDALMD_AOP_POINT);

			GDALClose(hDataset);
		}
	}

	if (opt.makeDTMFormat && dataSetOut) {
		if (dataSetOut->GetRasterBand(1)) {
			dataSetOut->GetRasterBand(1)->SetNoDataValue(-9999);
		}
	}


	return true;
}

static WORD* growTo16BitWORD(WORD* pixels, int width, int height, int numChannels, int shift)
{
	int tileSize = width * height;
	WORD* outputBits = new WORD[tileSize * numChannels];
	WORD* outPtr = (WORD*)outputBits;
	WORD* srcPtr = pixels;
	for (int j = 0; j < tileSize; ++j) {
		for (int k = 0; k < numChannels; ++k) {
			*outPtr = WORD(*srcPtr) << shift;
			++outPtr;
			++srcPtr;
		}
	}
	return outputBits;
}

bool IWriteGDALImpl::writeTile(const char* tileData)
{
	int i = _tileNum;

	uint32_t row = i / _tilesAcross;
	uint32_t col = i - (row * _tilesAcross);

	int left = col * _opt.tileWidth;
	int top = row * _opt.tileHeight;
	int right = col * _opt.tileWidth + _opt.tileWidth;
	int bottom = row * _opt.tileHeight + _opt.tileHeight;
	if (left < 0) {
		left = 0;
	}
	if (top < 0) {
		top = 0;
	}
	if (right > int(_opt.width)) {
		right = _opt.width;
	}
	if (bottom > int(_opt.height)) {
		bottom = _opt.height;
	}

	uint32_t actualWidth = right - left;
	uint32_t actualHeight = bottom - top;

	CPLErr eErr;

	GDALDataset* dataSet = GDALDataset::FromHandle(hOutDS);

#if 1

	// interleaved

	VOID* pixelsToUse = (VOID*)tileData;
	WORD* newPixels = nullptr;

	if (_opt.bytesPerChannel == 2 && _opt._compType == GDALWriteOptions::GW_JXL) {
		// must use full 16 bit for losssy compression
		if (_opt.bitsPerSample < 16) {
			newPixels = growTo16BitWORD((WORD*)tileData, actualWidth, actualHeight, _opt.numChannels, 16 - _opt.bitsPerSample);
			pixelsToUse = newPixels;
		}
	}

	eErr = GDALDatasetRasterIO(hOutDS, GF_Write, left, top, actualWidth, actualHeight,
		pixelsToUse,
		actualWidth, // nBufXSize
		actualHeight, // nBufYSize
		_dataType, // eBufType
		_opt.numChannels, // nBandCount
		NULL, // panBandMap 
		_opt.numChannels * _opt.bytesPerChannel, // nPixel Space
		actualWidth * _opt.numChannels * _opt.bytesPerChannel, // nLine Space, The byte offset from the start of one scanline in pData to the start of the next
		_opt.bytesPerChannel);

	if (newPixels)
		delete[] newPixels;
#else
	// bands

	int totalBytes = actualWidth * actualHeight * _opt.numChannels * _opt.bytesPerChannel;
	BYTE* newData = new BYTE[totalBytes];
	memset(newData, 128, totalBytes);

	int bandSize = actualWidth * actualHeight * _opt.bytesPerChannel;
	int lineSize = actualWidth * _opt.bytesPerChannel * _opt.numChannels;
	int bytesPerPixel = _opt.bytesPerChannel * _opt.numChannels;
	BYTE* newDataPtr = newData;
	BYTE* oldDataPtr = (BYTE*)tileData;

	int tileSize = actualWidth * actualHeight;
	for (int y = 0; y < tileSize; ++y) {
		for (int band = 0; band < _opt.numChannels; ++band) {
			BYTE* bandPtr = newDataPtr + (band * bandSize);
			BYTE* oldPtr = oldDataPtr + band * _opt.bytesPerChannel;

			for (int j = 0; j < _opt.bytesPerChannel; ++j) {
				//*(bandPtr + j) = tileData[y*lineSize + x * bytesPerPixel + band*_opt.bytesPerChannel + j];
				*(bandPtr + j) = *(oldPtr + j);
			}
		}

		newDataPtr += _opt.bytesPerChannel;
		oldDataPtr += _opt.numChannels * _opt.bytesPerChannel;
	}


	eErr = dataSet->RasterIO(GF_Write, left, top, actualWidth, actualHeight,
		(VOID*)newData,
		actualWidth, // nBufXSize
		actualHeight, // nBufYSize
		_dataType, // eBufType
		_opt.numChannels, // nBandCount
		NULL, // panBandMap 
		0, // nPixel Space
		0, // nLine Space, The byte offset from the start of one scanline in pData to the start of the next
		0);

#if 0
	newDataPtr = newData;

	for (int i = 0; i < _opt.numChannels; ++i) {
		GDALRasterBand* poBand;
		poBand = dataSet->GetRasterBand(i + 1);

		GDALRasterIOExtraArg sExtraArg;
		INIT_RASTERIO_EXTRA_ARG(sExtraArg);
		sExtraArg.eResampleAlg = GRIORA_Cubic;

		//memset(newData, 128, totalBytes);

		CPLErr err = poBand->RasterIO(GF_Write, left, top, actualWidth, actualHeight,
			(VOID*)(newDataPtr + (i * bandSize)),
			//(VOID*)(newDataPtr),
			actualWidth, // nBufXSize
			actualHeight, // nBufYSize
			_dataType, // eBufType,
			0, 0, &sExtraArg);
	}
#endif

	delete[] newData;
#endif

	++_tileNum;
	return true;
}
bool IWriteGDALImpl::finish(bool wasGood)
{
	GDALClose(hOutDS);

	hOutDS = NULL;

	if (wasGood && _opt.generateZoomLevels) {
		genZoomLevels();
	}

	return true;
}

int IWriteGDALImpl::tilesPerImage()
{
	return _tilesPerImage;
}

void IWriteGDALImpl::genZoomLevels()
{
	char** papszOpenOptions = nullptr;

	// GA_Update makes the overview happen internal - we want this most of the time
	GDALDatasetH hDataset = GDALOpen(_filename.c_str(), GA_Update);

	CSLDestroy(papszOpenOptions);
	papszOpenOptions = nullptr;

	if (hDataset == nullptr)
		return;

	int nMinSize = 256;
	int nLevelCount = 0;
	int anLevels[256] = {};
	const int nXSize = GDALGetRasterXSize(hDataset);
	const int nYSize = GDALGetRasterYSize(hDataset);
	int nOvrFactor = 1;
	while (DIV_ROUND_UP(nXSize, nOvrFactor) > nMinSize ||
		DIV_ROUND_UP(nYSize, nOvrFactor) > nMinSize)
	{
		nOvrFactor *= 2;
		anLevels[nLevelCount++] = nOvrFactor;
	}

	// Only HFA supports selected layers
	int* panBandList = nullptr;
	int nBandCount = 0;
	const char* pszResampling = "CUBIC";

	if (nBandCount > 0)
		CPLSetConfigOption("USE_RRD", "YES");

	GDALProgressFunc pfnProgress = GDALProgressCallback;
	ProgressDataForGdal data;
	data._callback = _callback;
	data._clientData = _clientData;
	//data._cancelEvent = _cancelEvent;

	if (nLevelCount > 0 &&
		GDALBuildOverviews(hDataset, pszResampling, nLevelCount, anLevels,
			nBandCount, panBandList, pfnProgress,
			&data) != CE_None)
	{
		printf("Overview building failed.\n");
	}

	GDALClose(hDataset);
	CPLFree(panBandList);
}

bool IWriteGDALImpl::createCopy(const char* srcFilename, const char* dstFilename,
	GDALCreateCopyOptions opt, const char* driverName)
{
	GDALDatasetH hDataset = GDALOpen(srcFilename, GA_ReadOnly);

	GDALProgressFunc pfnProgress = GDALProgressCallback;
	ProgressDataForGdal data;
	data._callback = _callback;
	data._clientData = _clientData;
	//data._cancelEvent = _cancelEvent;

	char** papszOptions = nullptr;
	//papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", opt.bigTiff ? "YES" : "NO");
	papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", "IF_NEEDED"); // use big tif if it think we need it
	if (opt.bigTiff) {
		// force BigTiff
		papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", "YES");
	}

	papszOptions = CSLSetNameValue(papszOptions, "NUM_THREADS", "4");

	switch (opt._compType)
	{
	case GDALCreateCopyOptions::GCC_LZW:
	{
		papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "LZW");
	}
	break;
	case GDALCreateCopyOptions::GCC_DEFLATE:
	{
		papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "DEFLATE");
	}
	break;
	case GDALCreateCopyOptions::GCC_LERC_DEFLATE:
	{
		papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "LERC_DEFLATE");
	}
	break;
	case GDALCreateCopyOptions::GCC_JXL:
	{
		papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "JXL");
		papszOptions = CSLSetNameValue(papszOptions, "JXL_LOSSLESS", opt._jxlLossless ? "YES" : "NO");
		// the amount of compression 1-9, The higher, the smaller file
		std::string amt = StdUtility::string_format("%d", opt._jxlEffort);
		papszOptions = CSLSetNameValue(papszOptions, "JXL_EFFORT", amt.c_str());



		// from GDAL - For lossy compression, the recommended range is [0.5,3]
		if (!opt._jxlLossless) {

			double dist = 0.5;
			dist += .25 * double(opt._jxlEffort - 1);
			std::string distStr = StdUtility::string_format("%.2f", dist);

			papszOptions = CSLSetNameValue(papszOptions, "JXL_DISTANCE", distStr.c_str());
			papszOptions = CSLSetNameValue(papszOptions, "JXL_ALPHA_DISTANCE", distStr.c_str());
		}
		else {
			papszOptions = CSLSetNameValue(papszOptions, "JXL_DISTANCE", "0");
			papszOptions = CSLSetNameValue(papszOptions, "JXL_ALPHA_DISTANCE", "0");
		}
	}
	break;
	case GDALCreateCopyOptions::GCC_JPEG:
	{
		//papszOptions = CSLSetNameValue(papszOptions, "NBITS", "8");
		papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "JPEG");
		//papszOptions = CSLSetNameValue(papszOptions, "PHOTOMETRIC", "RGB");
		papszOptions = CSLSetNameValue(papszOptions, "PHOTOMETRIC", "YCBCR");

		// this just does not seem to work for COG
		// COG automatically makes it YCbCr and ignores Quality...still we will allow this default JPEG...
		//std::string qualStr = StdUtility::string_format("%d", opt._jpegQuality);
		//papszOptions = CSLSetNameValue(papszOptions, "JPEG_QUALITY", qualStr.c_str()); // 1-100
		//papszOptions = CSLSetNameValue(papszOptions, "JPEG_QUALITY", "25"); // 1-100
		//papszOptions = CSLSetNameValue(papszOptions, "JPEGTABLESMODE", "1");
	}
	break;

	}

	GDALDriverH hDriver = GDALGetDriverByName(driverName);

	GDALDatasetH newDataSet = GDALCreateCopy(hDriver, dstFilename, hDataset, 0, papszOptions, pfnProgress, &data);

	GDALClose(hDataset);
	if (!newDataSet) return false;

	GDALClose(newDataSet);

	// 	if (wasGood && _opt.generateZoomLevels) {
	// 		genZoomLevels();
	// 	}

	return true;
}

bool IWriteGDALImpl::createCopy(const char* srcFilename, const char* dstFilename,
	char** papszOptions, const char* driverName)
{
	GDALDatasetH hDataset = GDALOpen(srcFilename, GA_ReadOnly);

	GDALProgressFunc pfnProgress = GDALProgressCallback;
	ProgressDataForGdal data;
	data._callback = _callback;
	data._clientData = _clientData;
	//data._cancelEvent = _cancelEvent;

	GDALDriverH hDriver = GDALGetDriverByName(driverName);

	papszOptions = CSLSetNameValue(papszOptions, "WRITE_METADATA_AS_TEXT", "NO");
	papszOptions = CSLSetNameValue(papszOptions, "WORLDFILE", "NO");

	GDALDatasetH newDataSet = GDALCreateCopy(hDriver, dstFilename, hDataset, 0, papszOptions, pfnProgress, &data);

	GDALClose(hDataset);
	if (!newDataSet) return false;

	GDALClose(newDataSet);

	return true;
}

bool IWriteGDALImpl::writeAsTiff(uint8_t* bits, int width, int height, const char* filename, int bytesPerPixel)
{
	uint32_t imageWidth = width;
	uint32_t imageHeight = height;

	uint32_t tilesWidth = 256;
	uint32_t tilesHeight = 256;

	uint32_t tilesAcross = (imageWidth + tilesWidth - 1) / tilesWidth;
	uint32_t tilesDown = (imageHeight + tilesHeight - 1) / tilesHeight;
	uint32_t tilesPerImage = tilesAcross * tilesDown;

	GDALWriteOptions opt;
	opt.bytesPerChannel = 1;
	opt.tileWidth = tilesWidth;
	opt.tileHeight = tilesHeight;
	opt.width = imageWidth;
	opt.height = imageHeight;

	opt.numChannels = 3;
	opt.bytesPerChannel = 1;
	switch (bytesPerPixel)
	{
	case 1:
		opt.numChannels = 1;
		opt.bytesPerChannel = 1;
		break;
	case 3:
		opt.numChannels = 3;
		opt.bytesPerChannel = 1;
		break;
	case 6:
		opt.numChannels = 3;
		opt.bytesPerChannel = 2;
		break;
	default:
		break;
	}

	
	opt.generateZoomLevels = false;

	start(filename, opt, nullptr, nullptr);

	for (uint32_t i = 0; i < tilesPerImage; ++i) {
		uint32_t row = i / tilesAcross;
		uint32_t col = i - (row * tilesAcross);

		int left = col * tilesWidth;
		int top = row * tilesHeight;
		int right = left + tilesWidth;
		int bottom = top + tilesHeight;

#pragma warning(suppress: 4018)
		if (right > imageWidth) {
			right = imageWidth;
		}
#pragma warning(suppress: 4018)
		if (bottom > imageHeight) {
			bottom = imageHeight;
		}

		uint32_t actualWidth = right - left;
		uint32_t actualHeight = bottom - top;

		uint8_t* dataPixel = new uint8_t[actualWidth * actualHeight * opt.numChannels * opt.bytesPerChannel];

		// copy data;
		uint8_t* scrPtr = bits;
		scrPtr += (left * opt.numChannels * opt.bytesPerChannel) + top * (imageWidth * opt.numChannels * opt.bytesPerChannel);

		uint8_t* dstPtr = dataPixel;
		int widthSize = actualWidth * opt.numChannels * opt.bytesPerChannel;
		for (int i = 0; i < actualHeight; ++i) {
			memcpy(dstPtr, scrPtr, widthSize);
			dstPtr += widthSize;
			scrPtr += imageWidth;
		}


		writeTile((char*)dataPixel);

		delete[] dataPixel;


	}

	finish(true);

	return false;
}