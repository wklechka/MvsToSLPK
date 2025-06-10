#include "pch.h"
#include "slpk3DTiles.h"
#include "Gen3DTiles.h"
#include "GenSLPK.h"
#include "StdUtil/StdUtility.h"
#include "Decimator.h"
#include "CoordSys.h"

class SLPK3DTilesImpl : public SLPK3DTiles
{
public:
	SLPK3DTilesImpl();
	~SLPK3DTilesImpl();

	virtual void Release() override;

	virtual bool generateSLPK(SlpkOptions& opt) override;
	virtual bool generate3DTiles(SlpkOptions& opt) override;

	virtual bool generateSplitFiles(SplitFilesOptions& opt) override;
};


SLPK_DLL SLPK3DTiles* createSLPK3DTiles()
{
	SLPK3DTilesImpl* impl = new SLPK3DTilesImpl;
	return impl;
}

SLPK3DTilesImpl::SLPK3DTilesImpl()
{}
SLPK3DTilesImpl::~SLPK3DTilesImpl()
{}

void SLPK3DTilesImpl::Release()
{
	delete this;
};


bool SLPK3DTilesImpl::generateSLPK(SlpkOptions& opt)
{
	return doVTK_SLPK(opt);
}
bool SLPK3DTilesImpl::generate3DTiles(SlpkOptions& opt)
{
	StdUtility::createFullDirectoryPath(opt.ouput3DTileFolder);
	return doVTK_3DTiles(opt);
}

bool SLPK3DTilesImpl::generateSplitFiles(SplitFilesOptions& opt)
{
	DecimatorOptions decOpt;
	decOpt.workingFolder = opt.workingFolder;
	decOpt.inputMeshFile = opt.inputMeshFile;
	decOpt.numLevels = opt.numLevels;
	decOpt.forceSquareTiles = opt.forceSquareTiles;
	decOpt.splitDivisions = opt.splitDivisions;

	return createDecimationLevelsVCG(decOpt, opt.lodSplitfiles);
}