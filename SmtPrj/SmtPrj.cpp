// SmtPrj.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "SmtPrj.h"
#include "SMXml.h"
#include "XMLCommon.h"
#include <fstream>
#include <algorithm>
#include <sstream>



constexpr int BUFF_SIZE = 512;

class CameraLoader
{
public:

	CameraLoader() { initMaps(); }

	bool loadCameraFile(const char* filename, ISmtPrj::CameraParameters &param)
	{
		param.hasDistortion = false;
		param.focal = 0.0;
		param.ppX = 0.0;
		param.ppY = 0.0;
		param.filmHeight = 0.0;
		param.filmWidth = 0.0;

		for (int i = 0; i < 8; ++i) {
			param.k[i] = 0.0;
		}
		for (int i = 0; i < 4; ++i) {
			param.p[i] = 0.0;
		}
		for (int i = 0; i < 2; ++i) {
			param.b[i] = 0.0;
		}
		param.cameraFile = "";


		char	buff[BUFF_SIZE + 1];
		memset(buff, 0, sizeof(buff));

		std::ifstream	file;

		if (filename == NULL) {
			return false;
		}

		file.open(filename);

		if (!file.is_open()) {
			return false;
		}

		param.cameraFile = filename;

		std::stringstream stream;

		// load the file into the stream
		CString data;
		while (file.getline(buff, BUFF_SIZE).good()) {
			data += buff;
			data += "\n";
		}
		stream.write(data, data.GetLength());

		loadCameraFile(stream, param);

		file.close();
		return true;
	}

	bool parseLine(const char* line, std::string& keyword, std::string& value)
	{
		char* str = (char*)line;
		char* keyEnd;

		keyword.clear();
		value.clear();

		while ((*str != ' ') && (*str != '\0') && (*str != '\t')) {
			++str;
		}

		keyEnd = str;

		CString tmp = line;
		keyword = tmp.Left(keyEnd - line);

		if (*str == '\0') {
			return false;
		}

		// eat white space
		while ((*str == ' ') && (*str != '\0') && (*str != '\t')) {
			++str;
		}

		if (*str == '\0') {
			return false;
		}

		value = str;

		return true;
	}

	bool loadCameraFile(std::stringstream& stream, ISmtPrj::CameraParameters& param)
	{
		char	buff[BUFF_SIZE + 1];
		memset(buff, 0, sizeof(buff));

		std::string keyWord;
		std::string value;

		while (stream.getline(buff, BUFF_SIZE).good()) {

			std::stringstream sStream;
			if (!parseLine(buff, keyWord, value)) {
				continue;
			}

			sStream << value.c_str();

			if (mKeyWordToItem.find(keyWord) != mKeyWordToItem.end()) {
				// the keyword was found
				switch (mKeyWordToItem[keyWord]) {
				case CAM_VERSION:
					break;
				case CAM_OWNER:
					break;
				case CAM_TYPE:
					break;
				case CAM_SN:
					break;
				case CAM_LENS_SN:
					break;
				case CAM_DATE:
					break;
				case CAM_NOTES:
					break;
				case CAM_FOCAL_LENGTH:
					sStream >> param.focal;
					break;
				case CAM_FILM_HEIGHT:
					sStream >> param.filmHeight;
					break;
				case CAM_FILM_WIDTH:
					sStream >> param.filmWidth;
					break;
				case CAM_FID_POINT:
					break;
				case CAM_PRINCIPAL_POINT:
					sStream >> param.ppX >> param.ppY;
					break;
				case CAM_CALIBRATION_TYPE:
				{
					int calType = atoi(value.c_str());
					if (calType == 8) {
						param.hasDistortion = true;
					}
				}
					break;
				case CAM_K1:
					break;
				case CAM_K3:
					break;
				case CAM_K5:
					break;
				case CAM_K7:
					break;
				case CAM_DISTANCE_VALUE:
					break;
				case CAM_ANGLE_VALUE:
					break;
				case CAM_DATA_STRIP_LOCATION:
					break;
				case CAM_DIGITAL_CAM_ROTATION:
					break;
				case CAM_IS_DIGITAL:
				break;
				case CAM_IS_FLIPPED:
				break;
				case CAM_FOV_WIDTH:
					break;
				case CAM_FOV_HEIGHT:
					break;
				case CAM_FOV_USE:
					break;

				case CAM_DECENTERING_PARAMS:
				break;
				case CAM_GRID_FILE:
				break;
				case CAM_DIST_7PARAM:
				break;
				case CAM_DIST_8PARAM:
				break;
				case CAM_DIST_12PARAM:
				{
					int i;
					for (i = 0; i < 8; ++i) {
						sStream >> param.k[i];
					}
					for (i = 0; i < 4; ++i) {
						sStream >> param.p[i];
					}
					for (i = 0; i < 2; ++i) {
						param.b[i] = 0.0;
					}
				}
				break;
				default:
					break;
				}
			}
		}

		return true;
	}

	enum ItemIdType {
		CAM_OWNER = 0,
		CAM_TYPE,
		CAM_SN,
		CAM_LENS_SN,
		CAM_DATE,
		CAM_FOCAL_LENGTH,
		CAM_FILM_HEIGHT,
		CAM_FILM_WIDTH,
		CAM_FID_POINT,
		CAM_PRINCIPAL_POINT,
		CAM_NOTES,
		CAM_CALIBRATION_TYPE,
		CAM_K1,
		CAM_K3,
		CAM_K5,
		CAM_K7,
		CAM_DISTANCE_VALUE,
		CAM_ANGLE_VALUE,
		CAM_DATA_STRIP_LOCATION,
		CAM_IS_FLIPPED,
		CAM_IS_DIGITAL,
		CAM_FOV_WIDTH,
		CAM_FOV_HEIGHT,
		CAM_FOV_USE,
		CAM_DECENTERING_PARAMS,
		CAM_DIGITAL_CAM_ROTATION,
		CAM_VERSION,
		CAM_GRID_FILE,
		CAM_DIST_7PARAM,
		CAM_DIST_8PARAM,
		CAM_DIST_12PARAM,
		CAM_NUM_ITEM_IDS
	};


	typedef std::map<std::string, ItemIdType> StringItemMap;

	StringItemMap mKeyWordToItem;
	void initMaps()
	{
		if (mKeyWordToItem.size() == 0) {
			mKeyWordToItem["OWNER:"] = CAM_OWNER;
			mKeyWordToItem["TYPE:"] = CAM_TYPE;
			mKeyWordToItem["SN:"] = CAM_SN;
			mKeyWordToItem["LENS_SN:"] = CAM_LENS_SN;
			mKeyWordToItem["DATE:"] = CAM_DATE;
			mKeyWordToItem["FOCAL_LENGTH:"] = CAM_FOCAL_LENGTH;
			mKeyWordToItem["FILM_HEIGHT:"] = CAM_FILM_HEIGHT;
			mKeyWordToItem["FILM_WIDTH:"] = CAM_FILM_WIDTH;
			mKeyWordToItem["FID_POINT:"] = CAM_FID_POINT;
			mKeyWordToItem["PRINCIPAL_POINT:"] = CAM_PRINCIPAL_POINT;
			mKeyWordToItem["NOTES:"] = CAM_NOTES;
			mKeyWordToItem["CALIBRATION_TYPE:"] = CAM_CALIBRATION_TYPE;
			mKeyWordToItem["K1:"] = CAM_K1;
			mKeyWordToItem["K3:"] = CAM_K3;
			mKeyWordToItem["K5:"] = CAM_K5;
			mKeyWordToItem["K7:"] = CAM_K7;
			mKeyWordToItem["DISTANCE_VALUE:"] = CAM_DISTANCE_VALUE;
			mKeyWordToItem["ANGLE_VALUE:"] = CAM_ANGLE_VALUE;
			mKeyWordToItem["DATA_STRIP_LOCATION:"] = CAM_DATA_STRIP_LOCATION;
			mKeyWordToItem["IS_DIGITAL:"] = CAM_IS_DIGITAL;
			mKeyWordToItem["IS_FLIPPED:"] = CAM_IS_FLIPPED;
			mKeyWordToItem["FOV_WIDTH:"] = CAM_FOV_WIDTH;
			mKeyWordToItem["FOV_HEIGHT:"] = CAM_FOV_HEIGHT;
			mKeyWordToItem["FOV_USE:"] = CAM_FOV_USE;
			mKeyWordToItem["DECENTERING:"] = CAM_DECENTERING_PARAMS;
			mKeyWordToItem["CAM_DIGITAL_CAM_ROTATION:"] = CAM_DIGITAL_CAM_ROTATION;
			mKeyWordToItem["CAM_VERSION:"] = CAM_VERSION;
			mKeyWordToItem["CAM_GRID_FILE:"] = CAM_GRID_FILE;
			mKeyWordToItem["DISTORTION_7PARAM:"] = CAM_DIST_7PARAM;
			mKeyWordToItem["DISTORTION_8PARAM:"] = CAM_DIST_8PARAM;
			mKeyWordToItem["DISTORTION_12PARAM:"] = CAM_DIST_12PARAM;
		}
	}
};



class ProjInfo
{
public:
	bool load(const char* filename = NULL)
	{
		char	buff[5000 + 1] = "";
		std::string	tmpString;

		std::ifstream	file;

		std::string	 fileToLoad;

		if (filename == NULL) {
			fileToLoad = _projectFile;
		}
		else {
			fileToLoad = filename;
		}

		if (fileToLoad.size() <= 0) {
			return false;
		}

		//clear();

		_projectFile = fileToLoad;

		SM_XML xmlFile;

		xmlFile.Load(fileToLoad.c_str());


		// parse the XML file

		initSM_XMLmaps();

		char nameBuffer[256];

		SM_XMLElement* root = xmlFile.GetRootElement();

		root->GetElementName(nameBuffer);

		unsigned int numChildren = root->GetChildrenNum();

		unsigned int i = 0;
		for (i = 0; i < numChildren; ++i) {
			SM_XMLElement* level1 = root->GetChildren()[i];

			level1->GetElementName(nameBuffer);
			std::string name = nameBuffer;

			if (_elemToItem.find(name) == _elemToItem.end()) {
				// ERROR - BAD or unknown TAG
				continue;
			}

			// this should alway be 1 or 0
			std::string val;
			if (level1->GetContentsNum() == 1) {
				char buff[2048];
				level1->GetContents()[0]->GetValue(buff);
				val = buff;
			}

			switch (_elemToItem[nameBuffer]) {
			case ProjectType_id:
				_projectType = val;
				break;
			case ProjectSubType_id:
				_projectTypeSub = val;
				break;
			case Version_id:
				_version = val;
				break;
			case ControlList_id:
				processChildren(level1, &ProjInfo::processControlListChild);
				break;
			case CameraList_id:
				processChildren(level1, &ProjInfo::processCameraListChild);
				break;
			

			case Images_id:
				processChildren(level1, &ProjInfo::processImageInfo);
				break;

			case ProjectZ_id:
				level1->findVariable("Use", _useProjectZ);
				level1->findVariable("AveZ", _projectZ);
				break;
			case Models_id:
				processChildren(level1, &ProjInfo::processModelInfo);
				break;
			}
		}

		fillCameraData();

		return true;
	}

	std::string& projectFile() { return _projectFile; }


	void xmlControlList(SM_XML* xmlFile)
	{
		SM_XMLElement* controlListElem = new SM_XMLElement(0, "ControlList");

		int i;
		for (i = 0; i < _controlList.size(); ++i) {
			SM_XMLElement* controlElem = new SM_XMLElement(0, "Control");
			addContent(controlElem, _controlList[i]);
			controlListElem->AddElement(controlElem);
		}

		xmlFile->GetRootElement()->AddElement(controlListElem);
	}
	void xmlCameraList(SM_XML* xmlFile)
	{
		SM_XMLElement* cameraListElem = new SM_XMLElement(0, "CameraList");

		int i;
		for (i = 0; i < _cameraList.size(); ++i) {
			SM_XMLElement* cameraElem = new SM_XMLElement(0, "Camera");
			addContent(cameraElem, _cameraList[i]);
			cameraListElem->AddElement(cameraElem);
		}

		xmlFile->GetRootElement()->AddElement(cameraListElem);
	}

	void xmlExterior(SM_XMLElement* xmlElem, ISmtPrj::ImageInfo& imageInfo)
	{
		if (imageInfo.hasEO) {
			SM_XMLElement* extElem = new SM_XMLElement(0, "Exterior");

			SM_XMLElement* elm = new SM_XMLElement(0, "AngleUnits");

			switch (imageInfo.angleUnits) {
			case ISmtPrj::DEGREES:
				addContent(elm, "DEGREES");
				break;
			case ISmtPrj::RADIANS:
				addContent(elm, "RADIANS");
				break;
			case ISmtPrj::GRADS:
				addContent(elm, "GRADS");
				break;
			}
			extElem->AddElement(elm);

			elm = new SM_XMLElement(0, "ProjectionCenter");
			CStringA tmp;
			tmp.Format("%f %f %f", imageInfo.projectionCenterX,
				imageInfo.projectionCenterY,
				imageInfo.projectionCenterZ);
			addContent(elm, tmp);
			extElem->AddElement(elm);


			elm = new SM_XMLElement(0, "Angles");
			tmp.Format("%.15f %.15f %.15f", imageInfo.omega,
				imageInfo.phi,
				imageInfo.kappa);
			addContent(elm, tmp);
			extElem->AddElement(elm);

			elm = new SM_XMLElement(0, "UseExterior");
			addContent(elm, imageInfo.useEO);
			extElem->AddElement(elm);


			xmlElem->AddElement(extElem);
		}
	}

	void xmlImageInfoAerial(SM_XMLElement* xmlElem, ISmtPrj::ImageInfo& imageInfo)
	{
		SM_XMLElement* elemCam = new SM_XMLElement(0, "AssignedCamera");
		addContent(elemCam, imageInfo.assignedCamera);
		xmlElem->AddElement(elemCam);

// 		xmlTiePoints(xmlElem, imageInfo);
// 		xmlGroundPoints(xmlElem, imageInfo);
		xmlExterior(xmlElem, imageInfo);
	}

	void xmlImageInfo(SM_XMLElement* xmlElem, ISmtPrj::ImageInfo& imageInfo)
	{
		SM_XMLElement* imageElement = new SM_XMLElement(0, "ImageInfo");

		SM_XMLElement* elm = new SM_XMLElement(0, "ImageFile");
		addContent(elm, imageInfo.imageFile);
		imageElement->AddElement(elm);

		if (imageInfo.imageHeight != 0 && imageInfo.imageWidth != 0) {
			elm = new SM_XMLElement(0, "Dimensions");
			CString tmp;
			tmp.Format("%d %d", imageInfo.imageWidth, imageInfo.imageHeight);
			addContent(elm, tmp);
			imageElement->AddElement(elm);
		}


		xmlImageInfoAerial(imageElement, imageInfo);

		xmlElem->AddElement(imageElement);
	}

	void xmlImages(SM_XML* xmlFile)
	{
		SM_XMLElement* elem = new SM_XMLElement(0, "Images");

		int i;
		for (i = 0; i < _imageList.size(); ++i) {
			xmlImageInfo(elem, _imageList[i]);
		}

		xmlFile->GetRootElement()->AddElement(elem);
	}

	SM_XMLElement* xmlModelInfo(SM_XMLElement* xmlElem, ISmtPrj::ModelInfo& modelInfo, bool writeGen)
	{
		SM_XMLElement* modelInfoElem = new SM_XMLElement(0, "ModelInfo");


		SM_XMLElement* elm = new SM_XMLElement(0, "ModelName");
		addContent(elm, modelInfo.modelName);
		modelInfoElem->AddElement(elm);

		elm = new SM_XMLElement(0, "ModelLeft");
		addContent(elm, modelInfo.leftImage);
		modelInfoElem->AddElement(elm);

		elm = new SM_XMLElement(0, "ModelRight");
		addContent(elm, modelInfo.rightImage);
		modelInfoElem->AddElement(elm);

		if (writeGen) {
			elm = new SM_XMLElement(0, "Generated");
			addContent(elm, modelInfo.wasGenerated);
			modelInfoElem->AddElement(elm);
		}

		xmlElem->AddElement(modelInfoElem);

		return modelInfoElem;
	}

	void xmlModels(SM_XML* xmlFile)
	{
		SM_XMLElement* elem = new SM_XMLElement(0, "Models");

		for (int i = 0; i < _modelList.size(); ++i) {
			xmlModelInfo(elem, _modelList[i], false);
		}

		xmlFile->GetRootElement()->AddElement(elem);
	}
	bool writeXML(const char* filename)
	{
		std::string fileToWrite;

		if (filename == NULL) {
			fileToWrite = _projectFile;
		}
		else {
			fileToWrite = filename;
		}

		if (fileToWrite.size() <= 0) {
			return false;
		}

		_projectFile = fileToWrite;


		SM_XML xmlFile;

		SM_XMLHeader* hdr = new SM_XMLHeader("\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>");
		xmlFile.SetHeader(hdr);

		SM_XMLElement* pElement = new SM_XMLElement(0, "SummitProject");
		xmlFile.SetRootElement(pElement);

		SM_XMLElement* elem = new SM_XMLElement(0, "ProjectType");
		CString tmp = "Aerial";
		addContent(elem, tmp);
		xmlFile.GetRootElement()->AddElement(elem);


		_version = "8.4";
		elem = new SM_XMLElement(0, "Version");
		addContent(elem, _version);

		elem->addVariable("dcn", 1); 


		xmlFile.GetRootElement()->AddElement(elem);

		xmlControlList(&xmlFile);
		xmlCameraList(&xmlFile);

		xmlImages(&xmlFile);

		xmlModels(&xmlFile);
		//xmlInternalCameras(&xmlFile);

		xmlFile.Save(fileToWrite.c_str());
		return true;
	}

protected:
	std::string _projectFile;
	
	std::vector<ISmtPrj::ImageInfo> _imageList;
	std::vector<ISmtPrj::ModelInfo> _modelList;

	std::vector<std::string> _cameraList;
	std::vector<ISmtPrj::CameraParameters> _cameraParam;

	std::vector<std::string> _controlList;

	std::string _projectType;
	std::string _projectTypeSub;
	std::string _version;

	ISmtPrj::ImageInfo _tmpImageInfo;
	ISmtPrj::ModelInfo _tmpModelInfo;

	double _projectZ = 0.0;
	bool _useProjectZ = false;

	void fillCameraParam(ISmtPrj::CameraParameters &param, std::string& cameraFile)
	{
		CameraLoader cameraLoader;
		cameraLoader.loadCameraFile(cameraFile.c_str(), param);
	}

	void fillCameraData()
	{
		_cameraParam.resize(_cameraList.size());
		for (int i = 0; i < _cameraList.size(); ++i) {
			fillCameraParam(_cameraParam[i], _cameraList[i]);
		}
	}

	void setCamera(ISmtPrj::ImageInfo& imageInfo, std::string& value)
	{
		imageInfo.assignedCamera = atoi(value.c_str());
		if (imageInfo.assignedCamera < 0 || imageInfo.assignedCamera >= _cameraList.size()) {
			imageInfo.assignedCamera = 0;
		}
	}

	void setAngleUnits(ISmtPrj::ImageInfo& imageInfo, std::string& value)
	{
		std::transform(value.begin(), value.end(), value.begin(), ::toupper);

		imageInfo.angleUnits = ISmtPrj::DEGREES;

		if (value == "RADIANS" || value == "RADIAN" || value == "RADS") {
			imageInfo.angleUnits = ISmtPrj::RADIANS;
		}
		if (value == "GRADS" || value == "GRAD" || value == "GRADIAN" || value == "GRADIANS") {
			imageInfo.angleUnits = ISmtPrj::GRADS;
		}
	}

	void setExteriorCenter(ISmtPrj::ImageInfo& imageInfo, std::string& value)
	{
		std::stringstream sStream;
		sStream << value.c_str();

		double x = 0;
		double y = 0;
		double z = 0;

		sStream >> x >> y >> z;

		imageInfo.projectionCenterX = x;
		imageInfo.projectionCenterY = y;
		imageInfo.projectionCenterZ = z;
	}

	void setExteriorAngles(ISmtPrj::ImageInfo& imageInfo, std::string& value)
	{
		std::stringstream sStream;
		sStream << value.c_str();

		double omega = 0;
		double phi = 0;
		double kappa = 0;

		sStream >> imageInfo.omega >> imageInfo.phi >> imageInfo.kappa;

		imageInfo.hasEO = true;
	}

	static void processExterior(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case AngleUnits_id:
			projInfo->setAngleUnits(projInfo->_tmpImageInfo, val);
			break;
		case ProjectionCenter_id:
			projInfo->setExteriorCenter(projInfo->_tmpImageInfo, val);
			break;
		case Angles_id:
			projInfo->setExteriorAngles(projInfo->_tmpImageInfo, val);
			break;
		case UseExterior_id:
			projInfo->_tmpImageInfo.useEO = atoi(val.c_str()) == 1;
			break;
		break;
		}
	}

	static void processImageInfoChild(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case ImageFile_id:
			projInfo->_tmpImageInfo.imageFile = val;
			break;
		case Dimensions_id:
			sscanf(val.c_str(), "%d %d", &projInfo->_tmpImageInfo.imageWidth, &projInfo->_tmpImageInfo.imageHeight);
			break;
		case AssignedCamera_id:
			projInfo->setCamera(projInfo->_tmpImageInfo, val);
			break;
		case Exterior_id:
			projInfo->processChildren(xmlElem, &ProjInfo::processExterior);
			break;
		}
	}

	static void processImageInfo(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case ImageInfo_id:
			projInfo->_tmpImageInfo.clear();
			projInfo->processChildren(xmlElem, &ProjInfo::processImageInfoChild);
			projInfo->_imageList.push_back(projInfo->_tmpImageInfo);
			break;
		}
	}

	static void processCameraListChild(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case Camera_id:
			projInfo->_cameraList.push_back(val);
			break;
		}
	}

	static void processControlListChild(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case Control_id:
			projInfo->_controlList.push_back(val);
			break;
		}
	}

	static void processModelInfoChild(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case ModelName_id:
			projInfo->_tmpModelInfo.modelName = val;
			break;
		case ModelLeft_id:
			projInfo->_tmpModelInfo.leftImage = val;
			break;
		case ModelRight_id:
			projInfo->_tmpModelInfo.rightImage = val;
			break;
		case Generated_id:
			projInfo->_tmpModelInfo.wasGenerated = atoi(val.c_str()) == 1;
			break;
		}
	}

	static void processModelInfo(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val)
	{
		switch (projInfo->_elemToItem[name]) {
		case ModelInfo_id:
			projInfo->_tmpModelInfo.clear();
			projInfo->processChildren(xmlElem, &ProjInfo::processModelInfoChild);
			projInfo->_modelList.push_back(projInfo->_tmpModelInfo);
			break;
		}
	}

	enum keywordIndexSM_XML {
		SummitProject_id = 0,
		ProjectType_id,
		ProjectSubType_id,
		Version_id,
	
		ControlList_id,
		Control_id,
		CameraList_id,
		Camera_id,

		Images_id,
		ImageInfo_id,
		ImageFile_id,
		Dimensions_id,
		AssignedCamera_id,
		Exterior_id,
		AngleUnits_id,
		ProjectionCenter_id,
		Angles_id,
		UseExterior_id,

		ProjectZ_id,

		Models_id,
		ModelInfo_id,
		ModelName_id,
		ModelLeft_id,
		ModelRight_id,
		Generated_id
	};
	void initSM_XMLmaps()
	{
		if (_elemToItem.size() != 0) return;

		_elemToItem["SummitProject"] = SummitProject_id;
		_elemToItem["ProjectType"] = ProjectType_id;
		_elemToItem["ProjectSubType"] = ProjectSubType_id;
		_elemToItem["Version"] = Version_id;

		_elemToItem["ControlList"] = ControlList_id;
		_elemToItem["Control"] = Control_id;
		_elemToItem["CameraList"] = CameraList_id;
		_elemToItem["Camera"] = Camera_id;

		_elemToItem["Images"] = Images_id;
		_elemToItem["ImageInfo"] = ImageInfo_id;
		_elemToItem["ImageFile"] = ImageFile_id;
		_elemToItem["Dimensions"] = Dimensions_id;
		_elemToItem["AssignedCamera"] = AssignedCamera_id;
		
		_elemToItem["Exterior"] = Exterior_id;
		_elemToItem["AngleUnits"] = AngleUnits_id;
		_elemToItem["ProjectionCenter"] = ProjectionCenter_id;
		_elemToItem["Angles"] = Angles_id;
		_elemToItem["UseExterior"] = UseExterior_id;

		_elemToItem["ProjectZ"] = ProjectZ_id;

		_elemToItem["Models"] = Models_id;
		_elemToItem["ModelInfo"] = ModelInfo_id;
		_elemToItem["ModelName"] = ModelName_id;
		_elemToItem["ModelLeft"] = ModelLeft_id;
		_elemToItem["ModelRight"] = ModelRight_id;
		_elemToItem["Generated"] = Generated_id;
	}
	std::map<std::string, keywordIndexSM_XML> _elemToItem;

	typedef void (*ProcessChildFunc)(ProjInfo* projInfo, SM_XMLElement* xmlElem, std::string& name, std::string& val);
	unsigned int processChildren(SM_XMLElement* xmlElem, ProcessChildFunc func)
	{
		char nameBuffer[256];
		unsigned int numChild = xmlElem->GetChildrenNum();
		unsigned int i = 0;
		for (i = 0; i < numChild; ++i) {
			SM_XMLElement* level1 = xmlElem->GetChildren()[i];

			level1->GetElementName(nameBuffer);
			std::string name = nameBuffer;

			if (_elemToItem.find(name) == _elemToItem.end()) {
				continue;
			}

			// this should alway be 1 or 0
			std::string val;
			if (level1->GetContentsNum() == 1) {
				char buff[50000];
				level1->GetContents()[0]->GetValue(buff);
				val = buff;
			}

			func(this, level1, name, val);
		}

		return numChild;
	}
};

double zFromControlFile(const std::vector<std::string> &controlList)
{
	if (controlList.size() == 0) {
		return 0.0;
	}

	double zSum = 0.0;
	int numPoints = 0;
	const int maxPoints = 5000;

	for (auto& filename : controlList) {
		std::ifstream file;
		file.open(filename.c_str());
		if (!file.is_open()) {
			continue;
		}
		char buff[BUFF_SIZE + 1];
		memset(buff, 0, sizeof(buff));
		while (file.getline(buff, BUFF_SIZE).good()) {
			
			std::string line = buff;

			std::stringstream stream;
			stream << line.c_str();

			std::string id, ptType;
			double x, y, z;
			stream >> id >> ptType >> x >> y >> z;
			
			zSum += z;
			++numPoints;

			if (numPoints >= maxPoints) {
				break;
			}
		}
		file.close();
	}

	double ave = numPoints > 0 ? zSum / double(numPoints) : 0.0;
	return ave;
}

class ProjectSmtImpl : public ISmtPrj, public ProjInfo
{
public:
	ProjectSmtImpl()
	{
	}
	~ProjectSmtImpl()
	{
	}
	void Release() override
	{
		delete this;
	}
	bool setFile(const char* filename) override
	{
		return ProjInfo::load(filename);
	}
	std::string projectFilename() override
	{
		return ProjInfo::projectFile();
	}

	std::string getProjectType() override
	{
		return _projectType;
	}

	int numImages() override
	{
		return int(_imageList.size());
	}

	std::string getImage(int imageNum) override
	{
		if (imageNum < 0 || imageNum >= _imageList.size())
			return "";

		return _imageList[imageNum].imageFile;
	}

	double averageGroundZ() override
	{
		if (_useProjectZ)
		{
			return _projectZ;
		}
		// if for some reason this is not set we can get
		// the average Z from the control file
		return zFromControlFile(_controlList);
	}

	#define M_PI       3.14159265358979323846
	inline double degreesToRadians(double angle)
	{
		return angle * M_PI / 180.0;
	}

	bool getEOInfo(int imageNum, ISmtPrj::EOImageInfo& eoInfo) override
	{
		if (imageNum < 0 || imageNum >= _imageList.size()) {
			return false;
		}

		ImageInfo& info = (_imageList[imageNum]);

		eoInfo.hasEO = info.hasEO;

		eoInfo.omega = info.omega;
		eoInfo.phi = info.phi;
		eoInfo.kappa = info.kappa;
		if (info.angleUnits == ISmtPrj::DEGREES) {
			eoInfo.omega = degreesToRadians(info.omega);
			eoInfo.phi = degreesToRadians(info.phi);
			eoInfo.kappa = degreesToRadians(info.kappa);
		}

		eoInfo.projCenterX = info.projectionCenterX;
		eoInfo.projCenterY = info.projectionCenterY;
		eoInfo.projCenterZ = info.projectionCenterZ;

		eoInfo.cameraIndex = info.assignedCamera;
		eoInfo.imageWidth = info.imageWidth;
		eoInfo.imageHeight = info.imageHeight;

		return true;
	}

	int numCameras() override
	{
		return int(_cameraList.size());
	}

	bool getCameraInfo(int cameraNum, ISmtPrj::CameraParameters& cam) override
	{
		if (cameraNum < 0 || cameraNum >= _cameraParam.size()) {
			return false;
		}

		cam = _cameraParam[cameraNum];

		return true;
	}


	bool writeProject(const char* filename) override
	{
		return writeXML(filename);
	}

	bool addImage(const ImageInfo& imageInfo) override
	{
		_imageList.push_back(imageInfo);

		return true;
	}

	bool addModel(const ModelInfo& modelInfo) override
	{
		_modelList.push_back(modelInfo);
		return true;
	}

	void clearImages() override
	{ 
		_imageList.clear();
	}
	void clearModels() override
	{
		_modelList.clear();
	}

	std::vector<ISmtPrj::ImageInfo>& getImageInfos() override
	{
		return _imageList;
	}
	std::vector<ISmtPrj::ModelInfo>& getModelInfos() override
	{
		return _modelList;
	}

	void forceValidModelList() override
	{
		// make sure all images in the models are in the image list

		std::vector<ModelInfo> newList;
		for (int i = 0; i < _modelList.size(); ++i) {
			ImageInfo tmpInfo;
			tmpInfo.imageFile = _modelList[i].leftImage;

			std::vector<ImageInfo>::iterator iter1;
			std::vector<ImageInfo>::iterator iter2;

			iter1 = find(_imageList.begin(), _imageList.end(), tmpInfo);
			tmpInfo.imageFile = _modelList[i].rightImage;
			iter2 = find(_imageList.begin(), _imageList.end(), tmpInfo);

			// if the images are in the image list
			if (iter1 != _imageList.end() && iter2 != _imageList.end()) {
				newList.push_back(_modelList[i]);
			}
		}

		if (newList.size() != _modelList.size()) {
			// ok fix the list
			_modelList = newList;
		}
	}
};

ISmtPrj* createISmtPrj()
{
	ISmtPrj* impl = new ProjectSmtImpl;

	return impl;
}

