#pragma once
#if !defined(IGDALWRITER_DLL)
#  if defined(_WIN32) && defined(IIMAGEREADER_BUILD_AS_DLL)
#    define IGDALWRITER_DLL     __declspec(dllexport)
#  else
#    define IGDALWRITER_DLL
#  endif
#endif

#include <stdint.h>

struct IImageFormat_GeoInfo
{
public:
	double _upperLeftGeo[2];
	double _lowerRightGeo[2];
	double _upperRightGeo[2];
	double _lowerLeftGeo[2];

	double _scaleX;
	double _scaleY;

	double _rotX;
	double _rotY;

	double _matrix[16];

	// project EPSG codes
	int _modelType;
	int _sys;
	int _datum;
	int _unitCode;

	enum PixelType { CENTER = 0, UPPER_LEFT };
	PixelType _pixelType;

	double _gpp;

	char _wkt[2048];
};

struct GDALWriteOptions
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t tileWidth = 256;
	uint32_t tileHeight = 256;
	int numChannels = 3;
	int bytesPerChannel = 1;
	int bitsPerSample = 8;
	int maxSampleValue = 0;

	bool hasGeoInfo = false;
	IImageFormat_GeoInfo geoInfo;

	bool generateZoomLevels = false;

	enum CompressionType { GW_NONE = 0, GW_JPEG, GW_JXL, GW_LERC, GW_LZW };
	CompressionType _compType = GW_NONE;

	// compression params
	bool jxlLossless = true;
	int jxlEffort = 5;

	bool _createBigTiff = false;

	bool makeDTMFormat = false;
};

struct GDALCreateCopyOptions
{
	bool bigTiff = false;
	enum CompressionType {
		GCC_LZW = 0, GCC_DEFLATE, GCC_LERC_DEFLATE, GCC_JXL, GCC_JPEG, GCC_NONE
	};
	CompressionType _compType = GCC_NONE;

	// JXL options
	bool _jxlLossless = true;
	int _jxlEffort = 5;

	// JPEG
	int _jpegQuality;
};

struct IWriteGDALProgress
{
	int start = 0;
	int stop = 0;
	int progress = 0;
};
typedef void(*IWriteGDALProgressCallback)(IWriteGDALProgress& info, void* clientData);

struct IImageWriter
{
	virtual void Release() = 0;

	virtual void setProgressCallback(IWriteGDALProgressCallback callback, void* clientData) = 0;
	virtual void cancelEvent(void* event) = 0;

	// writing images a tile at a time
	// driver must support create(), this writes GeoTiff though it could write others
	virtual bool start(const char* filename, const GDALWriteOptions& opt, const char* copyGeoInfoFromFile = nullptr, const char* driverName = nullptr) = 0;
	virtual bool writeTile(const char* tileData) = 0;
	virtual bool finish(bool wasGood = true) = 0;
	virtual int tilesPerImage() = 0;

	// writing whole image
	// drivername is the gdal name of the file type to create
	// driver must support createCopy()
	virtual bool createCopy(const char* srcFilename, const char* dstFilename,
		GDALCreateCopyOptions opt = GDALCreateCopyOptions(), const char* driverName = "COG") = 0;

	virtual bool createCopy(const char* srcFilename, const char* dstFilename,
		char** papszOptions = nullptr, const char* driverName = "PNG") = 0;

	virtual bool writeAsTiff(uint8_t* bits, int width, int height, const char* filename, int bytesPerPixel = 1) = 0;
};

extern "C" IGDALWRITER_DLL IImageWriter* createIImageWriter();