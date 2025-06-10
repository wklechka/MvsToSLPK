#pragma once
#if !defined(IIMAGEREADER_DLL)
#  if defined(_WIN32) && defined(IIMAGEREADER_BUILD_AS_DLL)
#    define IIMAGEREADER_DLL     __declspec(dllexport)
#  else
#    define IIMAGEREADER_DLL
#  endif
#endif

#include "IImageWriter.h"

#define IGDAL_MAX_STRING 256

class GDAL_RPCInfo
{
public:
	double _lineNumCoef[20];
	double _lineDenCoef[20];

	double _sampNumCoef[20];
	double _sampDenCoef[20];

	double _latOffset;
	double _longOffset;
	double _heightOffset;

	double _lineOffset;
	double _sampOffset;

	double _latScale;
	double _longScale;
	double _lineScale;
	double _sampScale;
	double _heightScale;
};

struct GDALImageInfo
{
	char _desc[IGDAL_MAX_STRING];
	char _longName[IGDAL_MAX_STRING];

	int _tileWidth;
	int _tileHeight;
	int _bytesPerPixel;
	int _bytesPerChannel;
	int _numChannels;
	int _bitsPerSample;
	int _imageWidth;
	int _imageHeight;
	int _numImages;

	bool _hasGeoInfo;
	double _adfGeoTransform[6];

	char _compressionMeth[IGDAL_MAX_STRING];

	//EPSG
	int _model;
	int _sys;
	int _datum;
	int _unitCode;

	bool _isJpeg;
	bool _isCOG;

	int _colorInterp;
	int _overviewCount;

	char _software[IGDAL_MAX_STRING];
	int _maxSampleVal;

	bool hasRPCData;
	GDAL_RPCInfo _rpcData;

	char _wkt[2048]; // spatial ref

	bool _hasEXIF;
};

struct IImageReader
{
	virtual void Release() = 0;

	virtual void ignoreUpperBands(bool val) = 0;

	virtual bool setFile(const char* filename) = 0;
	virtual bool getImageInfo(GDALImageInfo& info) = 0;

	virtual bool getImageInfo(const char* filename, GDALImageInfo& info) = 0;
	virtual bool getGeoInfo(IImageFormat_GeoInfo &geoInfo) = 0;

	virtual unsigned int readRectMemSize(int nXSize, int nYSize) = 0;
	virtual bool readRect(int nXOff, int nYOff, int nBufXSize, int nBufYSize, BYTE* data) = 0;

	// this read rect is always 1x to some buff size and is not affected by setImageNumber
	virtual bool readRect(int nXOff, int nYOff, int nXSize, int nYSize, int nBufXSize, int nBufYSize, BYTE* data) = 0;

	virtual bool setImageNumber(int num) = 0;
};

extern "C" IIMAGEREADER_DLL IImageReader* createIImageReader();

