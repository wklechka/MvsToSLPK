#pragma once
#include <stdint.h>

struct IWritePT
{
public:

	virtual void Release() = 0;

	virtual bool startWrite(const char* filename, double minX, double minY, double minZ, double maxX, double maxY, double maxZ) = 0;

	virtual void addPt(double x, double y, double z, uint32_t color, unsigned short intensity, unsigned char classification = 0,
		short flightLine = 0) = 0;

	virtual void finishWrite(bool updateHeader = true) = 0;
};

IWritePT* createIWritePT();


struct IReadPointData
{
	double x, y, z;
	uint32_t color;
	unsigned short intensity;
	unsigned char classification;
	short flightLine;
};

typedef bool(*IReadPTFileCallback)(IReadPointData& pointData, void* clientData);

struct IReadPT
{
public:

	virtual void Release() = 0;

	virtual bool addFilename(const char* filename) = 0;

	virtual void readPoints(IReadPTFileCallback callback, void* clientData) = 0;

};

IReadPT* createIReadPT();


