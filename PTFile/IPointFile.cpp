#include "pch.h"
#include "IPointFile.h"


class IWritePTFileImpl : public IWritePT
{
public:
	IWritePTFileImpl() {}
	~IWritePTFileImpl() {}

	virtual void Release() override { delete this; }

	virtual bool startWrite(const char* filename, double minX, double minY, double minZ, double maxX, double maxY, double maxZ) override
	{
		return false;
	}

	virtual void addPt(double x, double y, double z, uint32_t color, unsigned short intensity, unsigned char classification,
		short flightLine) override
	{}

	virtual void finishWrite(bool updateHeader) override
	{
	}

protected:
	// right now these are stubs
};

IWritePT* createIWritePT()
{
	IWritePTFileImpl* impl = new IWritePTFileImpl;
	return impl;
}


class IReadPTFileImpl : public IReadPT
{
public:
	virtual void Release()override { delete this; }
	virtual bool addFilename(const char* filename) override
	{
		return false;
	}

	virtual void readPoints(IReadPTFileCallback callback, void* clientData) override
	{
		_callback = callback;
		_clientData = clientData;
	}

private:
	IReadPTFileCallback _callback;
	void* _clientData;

};

IReadPT* createIReadPT()
{
	IReadPT* impl = new IReadPTFileImpl;
	return impl;
}