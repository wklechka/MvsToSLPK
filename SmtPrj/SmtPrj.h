#pragma once
#if !defined(ISMTPRJ_DLL)
#  if defined(_WIN32) && defined(ISMTPRJ_BUILD_AS_DLL)
#    define ISMTPRJ_DLL     __declspec(dllexport)
#  else
#    define ISMTPRJ_DLL
#  endif
#endif

#include <string>
#include <vector>

struct ISmtPrj
{
public:

	virtual void Release() = 0;

	// Reading and getting data from the project
	// load a summit project file
	virtual bool setFile(const char* filename) = 0;
	virtual std::string projectFilename() = 0;
	virtual std::string getProjectType() = 0;

	virtual int numImages() = 0;
	virtual std::string getImage(int imageNum) = 0;

	virtual double averageGroundZ() = 0;
	//virtual bool getModelBounds(double groundZ, std::vector<std::vector<double>>& result) = 0;

	struct EOImageInfo {
		bool hasEO;
		double projCenterX, projCenterY, projCenterZ;
		double omega, phi, kappa; // radians
		int cameraIndex;
		int imageWidth, imageHeight;
	};
	virtual bool getEOInfo(int imageNum, EOImageInfo& eoInfo) = 0;

	virtual int numCameras() = 0; // usually one
	struct CameraParameters
	{
		double focal; //mm
		double ppX, ppY; //mm
		double filmWidth; //mm
		double filmHeight; //mm
		std::string cameraFile;


		bool hasDistortion;
		// k0 - k7
		double k[8];
		// decentering/tangential
		double p[4];
		// Affinity
		double b[2];
	};
	virtual bool getCameraInfo(int cameraNum, CameraParameters& cam) = 0;

	



	// write the project file
	virtual bool writeProject(const char* filename) = 0;
	
	enum AngleType { DEGREES = 0, RADIANS, GRADS };

	class ImageInfo {
	public:
		ImageInfo() {}
		ImageInfo(const ImageInfo& rhs) { *this = rhs; }

		void clear() {
			imageFile = "";
			imageWidth = 0;
			imageHeight = 0;
			assignedCamera = -1;
			useEO = false;
			hasEO = false;
			projectionCenterX = projectionCenterY = projectionCenterZ = 0.0;
			omega = phi = kappa = 0.0;
			angleUnits = ISmtPrj::DEGREES;
		}

		std::string imageFile;

		int imageWidth;
		int imageHeight;

		// camera index
		int assignedCamera;


		// Exterior Info
		bool		useEO;	// whether to use the exterior
		bool		hasEO;	// whether an exterior has been set
		double	projectionCenterX, projectionCenterY, projectionCenterZ;
		double		omega;
		double		phi;
		double		kappa;
		ISmtPrj::AngleType	angleUnits;

		bool operator == (const ImageInfo& rhs) const { return imageFile == rhs.imageFile; }
		bool operator != (const ImageInfo& rhs) const { return imageFile != rhs.imageFile; }
		bool operator < (const ImageInfo& rhs) const { return imageFile < rhs.imageFile; }
		bool operator > (const ImageInfo& rhs) const { return imageFile > rhs.imageFile; }
	};

	class ModelInfo
	{
	public:
		ModelInfo() { clear(); }

		void clear()
		{
			modelName = "";
			leftImage = "";
			rightImage = "";
			wasGenerated = false;
		}

		std::string modelName;
		std::string leftImage;
		std::string rightImage;
		bool wasGenerated;

		bool operator == (const ModelInfo& rhs) const { return modelName == rhs.modelName; }
		bool operator != (const ModelInfo& rhs) const { return modelName != rhs.modelName; }
		bool operator < (const ModelInfo& rhs) const { return modelName < rhs.modelName; }
		bool operator > (const ModelInfo& rhs) const { return modelName > rhs.modelName; }
	};

	virtual bool addImage(const ImageInfo& imageInfo) = 0;
	virtual bool addModel(const ModelInfo& modelInfo) = 0;
	virtual void clearImages() = 0;
	virtual void clearModels() = 0;

	virtual std::vector<ISmtPrj::ImageInfo> &getImageInfos() = 0;
	virtual std::vector<ISmtPrj::ModelInfo> &getModelInfos() = 0;

	virtual void forceValidModelList() = 0;
};

extern "C" ISMTPRJ_DLL ISmtPrj* createISmtPrj();