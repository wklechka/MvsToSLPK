#include "pch.h"


#include "Gen3DTiles.h"
#include "UtilityFunc.h"

#include <iosfwd>
#include <sstream>

#include "VTKProcess.h"
#include "StdUtil\StdUtility.h"

// #include <vtkActor.h>
// #include <vtkCamera.h>
// #include <vtkLookupTable.h>
// #include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOBBDicer.h>
// #include <vtkOutlineCornerFilter.h>
// #include <vtkPolyDataMapper.h>
// #include <vtkProperty.h>
// #include <vtkRenderWindow.h>
// #include <vtkRenderWindowInteractor.h>
// #include <vtkRenderer.h>

#include <vtkBYUReader.h>
#include <vtkOBJReader.h>
#include <vtkOBJWriter.h>
#include <vtkPLYReader.h>
#include <vtkWriter.h>
#include <vtkPLYWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkSTLReader.h>
#include <vtkVector.h>
#include <vtkPointData.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkStringArray.h>
#include <vtkGLTFWriter.h>
#include "vtksys\FStream.hxx"
#include <vtkPolyDataNormals.h>
#include <vtkTransformFilter.h>
#include <vtkCellData.h>
#include <vtkTransform.h>
#include <vtkDoubleArray.h>
#include "FolderNames.h"

// #include <vtkSphereSource.h>
//#include <vtkXMLPolyDataReader.h>
// #include <vtksys/SystemTools.hxx>

#include <vtkCesium3DTilesWriter.h>
#include <vtkCesium3DTilesReader.h>

#include <vtk_libproj.h>
#include "vtkLogger.h"
#include "vtkAbstractArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"

#include "CoordSys.h"

#define TilesFolder "Tiles"

std::array<double, 6> ToLonLatRadiansHeight(const char* crs, const std::array<double, 6>& bb);

struct BoundsInfo
{
	// x min/max, y min/max, z min/max
	std::array<double, 6> bb = { std::numeric_limits<double>::max(),
	std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
	std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
	std::numeric_limits<double>::lowest() };

	std::string content;
	double geoError = 0.0;
};

struct TreeNode
{
	TreeNode* parent = nullptr; // if you have no parent you are the root
	std::vector<TreeNode*> _children;

	bool IsLeaf() { return _children.size() == 0; }
	bool IsRoot() { return parent == nullptr; }

	BoundsInfo info;
	static std::string crs;
	double LOD = 1.0;
};
std::string TreeNode::crs;

static void PostOrderTraversal(
	void (*Visit)(TreeNode* node, void* aux),
	TreeNode* node, void* aux)
{
	if (!node->IsLeaf())
	{
		for (int i = 0; i < node->_children.size(); i++)
		{
			PostOrderTraversal(Visit, node->_children[i], aux);
		}
	}
	(*Visit)(node, aux);
}

static void PreOrderTraversal(
	void (*Visit)(TreeNode* node, void* aux),
	TreeNode* node, void* aux)
{
	(*Visit)(node, aux);
	if (!node->IsLeaf())
	{
		for (int i = 0; i < node->_children.size(); i++)
		{
			PreOrderTraversal(Visit, node->_children[i], aux);
		}
	}
}

static void deleteTree(TreeNode* node)
{
	if (node->IsLeaf() && !node->IsRoot()) {
		delete node;
		return;
	}

	for (int i = 0; i < node->_children.size(); ++i) {
		deleteTree(node->_children[i]);
	}

	if (!node->IsRoot()) {
		delete node;
	}
	else {
		node->_children.clear();
	}
}

double ComputeGeometricErrorTilesetMesh(BoundsInfo& bound);
double GetRootLength2(BoundsInfo& bound);

static void SetField(vtkDataObject* obj, const char* name, const std::vector<std::string>& values)
{
	vtkFieldData* fd = obj->GetFieldData();
	if (!fd)
	{
		vtkNew<vtkFieldData> newfd;
		obj->SetFieldData(newfd);
		fd = newfd;
	}
	vtkNew<vtkStringArray> sa;
	sa->SetNumberOfTuples(values.size());
	for (size_t i = 0; i < values.size(); ++i)
	{
		const std::string& value = values[i];
		sa->SetValue(i, value);
	}
	sa->SetName(name);
	fd->AddArray(sa);
}

static bool writeToGlb(vtkSmartPointer<vtkPolyData>& mesh, const std::string& meshFilename, const std::string& outputFolder, int index, int lod)
{
	std::string newOutFolder = outputFolder;
	newOutFolder += "/";
	newOutFolder += TilesFolder;
	newOutFolder += "/" LODname_;
	newOutFolder += std::to_string(lod);
	StdUtility::createFullDirectoryPath(newOutFolder);

	std::string glbFilename = newOutFolder;
	glbFilename += "/";
	glbFilename += std::to_string(index);
	glbFilename += ".glb";

	std::string foundPngFile = StdUtility::replaceExtension(meshFilename, "png");
// 	if (!getSplitPNG(meshFilename, foundPngFile)) {
// 		return false;
// 	}

	// PNG should areadly be with the split files
	// copy png
	std::string newPngName = newOutFolder;
	newPngName += "\\";
	newPngName += std::to_string(index);
	newPngName += ".png";

	StdUtility::copyFile(foundPngFile, newPngName, false);

	std::vector<std::string> mergedFileNames;
	mergedFileNames.push_back(StdUtility::getName(newPngName, false));

	SetField(mesh, "texture_uri", mergedFileNames);

#if 0
	vtkNew<vtkPolyDataNormals> normals;
	vtkNew<vtkDoubleArray> normalsArray;
	normalsArray->SetNumberOfComponents(3); // 3d normals (ie x,y,z)
	normalsArray->SetNumberOfTuples(mesh->GetNumberOfPoints());

	mesh->GetCellData()->SetNormals(normalsArray);

	///////// Get cell normals ///////////
  // vtkSmartPointer<vtkDoubleArray> cellNormalsRetrieved =
  //    dynamic_cast<vtkDoubleArray*>(polydata->GetCellData()->GetNormals());
	auto cellNormalsRetrieved =
		dynamic_cast<vtkDoubleArray*>(mesh->GetCellData()->GetNormals());
	if (cellNormalsRetrieved)
	{
		cout << "There are " << cellNormalsRetrieved->GetNumberOfTuples()
			<< " cell normals." << endl;

		for (vtkIdType i = 0; i < cellNormalsRetrieved->GetNumberOfTuples(); i++)
		{
			double cN[3];
			cellNormalsRetrieved->GetTuple(i, cN);
			cout << "Cell normal " << i << ": " << cN[0] << " " << cN[1] << " "
				<< cN[2] << endl;
		}
	}
	else
	{
		cout << "No cell normals." << endl;
	}
#endif


	// create the GLB
	  // store tileMesh into a multiblock
	vtkNew<vtkMultiBlockDataSet> buildings;
	vtkNew<vtkMultiBlockDataSet> building;
	buildings->SetNumberOfBlocks(1);
	building->SetNumberOfBlocks(1);
	buildings->SetBlock(0, building);
	building->SetBlock(0, mesh);

	// write tileMesh to GLTF
	vtkNew<vtkGLTFWriter> writer;
	writer->RelativeCoordinatesOn();
	writer->SetInputData(buildings);
	writer->SetFileName(glbFilename.c_str());

	writer->SetTextureBaseDirectory(newOutFolder.c_str());

	//writer->SetPropertyTextureFile(StdUtility::getName(foundPngFile,false).c_str());
	writer->SetCopyTextures(false);
	writer->SetSaveTextures(true); // saves textures
	//writer->SetInlineData(false);
	writer->SetSaveNormal(true);

	// 	if (aux->SelectionField == vtkSelectionNode::CELL)
	// 	{
	// 		writer->SetSaveNormal(true);
	// 	}

	writer->Write();

	return true;
}



static std::array<double, 6> ExpandBounds(double* first, double* second)
{
	return { std::min(first[0], second[0]), std::max(first[1], second[1]),
	  std::min(first[2], second[2]), std::max(first[3], second[3]), std::min(first[4], second[4]),
	  std::max(first[5], second[5]) };
}


static nlohmann::json GenerateTileJson(TreeNode* node)
{
	nlohmann::json tree;
	nlohmann::json v;
	std::array<double, 6> nodeBounds = node->info.bb;
	std::array<double, 6> lonLatRadiansHeight = ToLonLatRadiansHeight(TreeNode::crs.c_str(), nodeBounds);
	std::ostringstream ostr;
	for (int i = 0; i < 6; ++i)
	{
		v[i] = lonLatRadiansHeight[i];
	}
	tree["boundingVolume"]["region"] = v;
	tree["geometricError"] = node->info.geoError;
	if (node->IsRoot())
	{
		tree["refine"] = "REPLACE";

		// gltf y-up to 3d-tiles z-up transform
		std::array<double, 16> t = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
			  0.0, 0.0, 1.0 };
		tree["transform"] = t;
	}
	// generate json for the node
	if (!node->IsLeaf())
	{
		if (!node->IsRoot()) {
			ostr.str("");
			ostr << node->info.content;
			tree["content"]["uri"] = ostr.str();
		}
		v.clear();
		for (int i = 0, j = 0; i < node->_children.size(); i++)
		{
			v[j++] = GenerateTileJson(node->_children[i]);
			tree["children"] = v;
		}
	}
	else
	{
		// LEAF

		ostr.str("");
		ostr << node->info.content;
		tree["content"]["uri"] = ostr.str();

		// 		ostr.str("");
		// 		ostr << node->GetID() << "/" << node->GetID() << this->ContentTypeExtension();
		// 		tree["content"]["uri"] = ostr.str();

	}
	return tree;
}

static void writeJson(TreeNode* root, const std::string& output, double fullGeoError)
{
	nlohmann::json RootJson;
	nlohmann::json v;
	RootJson["asset"]["version"] = "1.0";

	std::string content_gltf = "3DTILES_content_gltf";
	std::string mesh_gpu_instancing = "EXT_mesh_gpu_instancing";
	std::string extensionsUsed = "extensionsUsed";
	std::string extensionsRequired = "extensionsRequired";
	v = { content_gltf };
	RootJson[extensionsUsed] = v;
	RootJson[extensionsRequired] = v;
	v = { mesh_gpu_instancing };
	RootJson["extensions"][content_gltf][extensionsUsed] = v;
	RootJson["extensions"][content_gltf][extensionsRequired] = v;

	RootJson["geometricError"] = fullGeoError;
	RootJson["root"] = GenerateTileJson(root);
	vtksys::ofstream file(output.c_str());
	if (!file)
	{
		vtkLog(ERROR, "Cannot open " << output << " for writing");
		return;
	}
	file << std::setw(4) << RootJson << std::endl;
}




static double GetRootLength2(BoundsInfo& bound)
{
	std::array<double, 6>& bb = bound.bb;
	std::array<double, 3> length = { { bb[1] - bb[0], bb[3] - bb[2], bb[5] - bb[4] } };
	return length[0] * length[0] + length[1] * length[1] + length[2] * length[2];
}

//------------------------------------------------------------------------------
static double ComputeGeometricErrorTilesetMesh(BoundsInfo& bound)
{
	double length2 = GetRootLength2(bound);
	return std::sqrt(length2);
}

static void addBoundsInfo(TreeNode* newNode, vtkSmartPointer<vtkPolyData>& mesh, const std::string& meshFilename, int fileIndex, int lod)
{
	BoundsInfo& info = newNode->info;

	for (int i = 0; i < mesh->GetNumberOfCells(); ++i)
	{
		double bb[6];
		mesh->GetCell(i)->GetBounds(bb);
		info.bb = ExpandBounds(info.bb.data(), bb);
	}

	info.geoError = 0.0;


	std::string newOutFolder = TilesFolder;
	newOutFolder += "/" LODname_;
	newOutFolder += std::to_string(lod);

	std::string glbFilename = newOutFolder;
	glbFilename += "/";
	glbFilename += std::to_string(fileIndex);
	glbFilename += ".glb";

	info.content = glbFilename;
}

template <typename T>
vtkSmartPointer<T> TranslateMeshOrPoints(T* rootPoints, const double* fileOffset)
{
	vtkSmartPointer<T> ret;
	vtkNew<vtkTransformFilter> f;
	vtkNew<vtkTransform> t;
	t->Identity();
	t->Translate(fileOffset);
	f->SetTransform(t);
	f->SetInputData(rootPoints);
	// generate normals - these are needed in Cesium if there are no textures
	vtkNew<vtkPolyDataNormals> normals;
	normals->SetInputConnection(f->GetOutputPort());
	normals->Update();
	ret = T::SafeDownCast(normals->GetOutputDataObject(0));
	return ret;
}

#if 0
static bool ConvertDataSetCartesian(vtkPointSet* pointSet, const char* crs)
{
	if (!crs) return false;
	if (strlen(crs) == 0) return false;

	PJ* P;
	P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, crs, "+proj=cart", nullptr);
	if (P == nullptr)
	{
		vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
		return false;
	}
	/* For that particular use case, this is not needed. */
	/* proj_normalize_for_visualization() ensures that the coordinate */
	/* order expected and returned by proj_trans() will be longitude, */
	/* latitude for geographic CRS, and easting, northing for projected */
	/* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
	/* had been used, this might had been necessary. */
	PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
	if (P_for_GIS == nullptr)
	{
		proj_destroy(P);
		vtkLog(
			ERROR, "proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
		return false;
	}
	proj_destroy(P);
	P = P_for_GIS;

	// transform points to Cartesian coordinates
	vtkDataArray* points = pointSet->GetPoints()->GetData();
	vtkNew<vtkDoubleArray> newPoints;
	vtkDoubleArray* da = vtkArrayDownCast<vtkDoubleArray>(points);
	vtkFloatArray* fa = vtkArrayDownCast<vtkFloatArray>(points);
	bool conversion = false;
	if (!da)
	{
		if (fa)
		{
			//vtkLog(WARNING, "Converting float to double points.");
			newPoints->DeepCopy(fa);
			da = newPoints;
			conversion = true;
		}
		else
		{
			vtkLog(ERROR, "Points are not float or double.");
			return false;
		}
	}
	double* d = da->GetPointer(0);
	int n = da->GetNumberOfTuples();
	proj_trans_generic(P, PJ_FWD, d, sizeof(d[0]) * 3, n, d + 1, sizeof(d[0]) * 3, n, d + 2,
		sizeof(d[0]) * 3, n, nullptr, 0, 0);
	pointSet->GetPoints()->Modified();
	if (conversion)
	{
		pointSet->GetPoints()->SetData(newPoints);
	}
	proj_destroy(P);
	return true;
}
#endif

static void ConvertDataSetCartesian2(vtkSmartPointer<vtkPolyData>& mesh, const char* crs)
{
	vtkSmartPointer<vtkPoints> points = mesh->GetPoints();
	vtkSmartPointer<vtkDataArray> dataArrayPoints = points->GetData();
	vtkIdType numPoints = mesh->GetNumberOfPoints();

	CoordConverter cc;
	cc.setString(crs);

	for (vtkIdType i = 0; i < numPoints; i++)
	{
		vtkVector3d vertex;
		vertex[0] = dataArrayPoints->GetComponent(i, 0);
		vertex[1] = dataArrayPoints->GetComponent(i, 1);
		vertex[2] = dataArrayPoints->GetComponent(i, 2);

		cc.transformECEF(vertex[0], vertex[1], vertex[2]);

		double testX = vertex[0];
		double testY = vertex[1];
		double testZ = vertex[2];

		dataArrayPoints->SetComponent(i, 0, vertex[0]);
		dataArrayPoints->SetComponent(i, 1, vertex[1]);
		dataArrayPoints->SetComponent(i, 2, vertex[2]);
	}
}

static std::array<double, 6> ToLonLatRadiansHeight(const char* crs, const std::array<double, 6>& bb)
{
	std::array<double, 6> lonlatheight;
	lonlatheight[4] = bb[4];
	lonlatheight[5] = bb[5];

// 	if (!crs) return lonlatheight;
// 	if (strlen(crs) == 0) return lonlatheight;

	CoordConverter cc;
	if (crs) {
		cc.setString(crs);
	}
	

	for (size_t i = 0; i < 2; ++i)
	{
		double x = bb[i];
		double y = bb[i + 2];
		double z = bb[i + 4];
		cc.transform(x, y, z);

		lonlatheight[2 * i] = vtkMath::RadiansFromDegrees(x);
		lonlatheight[2 * i + 1] = vtkMath::RadiansFromDegrees(y);
		lonlatheight[i + 4] = z;
	}

#if 0

	std::ostringstream ostr;
	PJ* P;
	P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, crs, "+proj=longlat +ellps=WGS84 lon_0=0", nullptr);
	if (P == nullptr)
	{
		vtkLog(ERROR, "proj_create_crs_to_crs failed: " << proj_errno_string(proj_errno(nullptr)));
		return lonlatheight;
	}
	{
		/* For that particular use case, this is not needed. */
		/* proj_normalize_for_visualization() ensures that the coordinate */
		/* order expected and returned by proj_trans() will be longitude, */
		/* latitude for geographic CRS, and easting, northing for projected */
		/* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
		/* had been used, this might had been necessary. */
		PJ* P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
		if (P_for_GIS == nullptr)
		{
			proj_destroy(P);
			vtkLog(ERROR,
				"proj_normalize_for_visualization failed: " << proj_errno_string(proj_errno(nullptr)));
			return lonlatheight;
		}
		proj_destroy(P);
		P = P_for_GIS;
	}
	PJ_COORD c = { { 0, 0, 0, 0 } }, c_out;
	for (size_t i = 0; i < 2; ++i)
	{
		c.xy.x = bb[i];
		c.xy.y = bb[i + 2];
		c_out = proj_trans(P, PJ_FWD, c);
		lonlatheight[2 * i] = vtkMath::RadiansFromDegrees(c_out.lp.lam);
		lonlatheight[2 * i + 1] = vtkMath::RadiansFromDegrees(c_out.lp.phi);
	}
	proj_destroy(P);
	// std::cout << lonlatheight[0] << " "
	//           << lonlatheight[1] << " "
	//           << lonlatheight[2] << " "
	//           << lonlatheight[3] << std::endl;

#endif
	return lonlatheight;
}

static void meshOffsetPoints(vtkSmartPointer<vtkPolyData> &mesh, double offX, double offY, double offZ)
{
	vtkSmartPointer<vtkPoints> points = mesh->GetPoints();
	vtkSmartPointer<vtkDataArray> dataArrayPoints = points->GetData();
	vtkIdType numPoints = mesh->GetNumberOfPoints();

	for (vtkIdType i = 0; i < numPoints; i++)
	{
		vtkVector3d vertex;
		vertex[0] = dataArrayPoints->GetComponent(i, 0);
		vertex[1] = dataArrayPoints->GetComponent(i, 1);
		vertex[2] = dataArrayPoints->GetComponent(i, 2);

		dataArrayPoints->SetComponent(i, 0, vertex[0] + offX);
		dataArrayPoints->SetComponent(i, 1, vertex[1] + offY);
		dataArrayPoints->SetComponent(i, 2, vertex[2] + offZ);
	}
}

static bool writeAs3DTiles(SlpkOptions& opt)
{
	if (opt.lodSplitfiles.size() == 0)
		return false;

	double boxData[9];
	bool hadBox = slpkUtilityFunc::boxFile(opt.boxFilename, boxData);

	std::string prj_file_name = opt.prjFilename;
	std::string crs = slpkUtilityFunc::GetProjectionWKTSTring(prj_file_name);


	slpkUtilityFunc::setupProjDB();

	TreeNode::crs = crs;
	TreeNode _treeRoot;

	int fileIndex = 0;
	int lodLevel = 1;
	int lastLevel = opt.lodSplitfiles.size() - 1;

	// going backwards should be easier...
	std::vector<TreeNode*> upperLevelNodes;
	upperLevelNodes.push_back(&_treeRoot);

	// inverting will make it more like ESRI
	// level 1 is most detail and detail reduces in the children
	// 3D tiles is normally the opposite
#//define INVERT_IT


	// https://docs.ogc.org/cs/22-025r4/22-025r4.html#toc15
	// A tile’s geometric error defines the selection metric for that tile. Its value is a nonnegative number that specifies the error, in meters, 
	// of the tile’s simplified representation of its source geometry. Generally, the root tile will have the largest geometric error, 
	// and each successive level of children will have a smaller geometric error than its parent, with leaf tiles having a geometric error of or close to 0.
	// 
//#ifdef INVERT_IT
// 	for (int count = 0; count < lodSplitfiles.size(); ++count) {
// #else
	for (int count = opt.lodSplitfiles.size() - 1; count >= 0; --count) {
		//#endif
		auto& lodLevelFiles = opt.lodSplitfiles[count];

		std::vector<TreeNode*> nextLevelNodes;
		int meshIndex = 0;

		for (auto& meshObjFile : lodLevelFiles) {

			std::cout << "3DTile processing: " << StdUtility::getName(meshObjFile).c_str() << std::endl;

			vtkAbstractPolyDataReader* reader = nullptr;

			vtkNew<vtkOBJReader> objReader;
			vtkNew<vtkPLYReader> plyReader;

			bool isPly = StdUtility::CompareNoCase(StdUtility::getExtension(meshObjFile), "ply") == 0;
			if (isPly) {
				reader = plyReader;
			}
			else {
				reader = objReader;
			}

			reader->SetFileName(meshObjFile.c_str());
			reader->Update();

			vtkSmartPointer<vtkPolyData> mesh = reader->GetOutput();

			// New offset mesh
			//double off[3] = { boxData[0], boxData[1], boxData[2] };
			//vtkSmartPointer<vtkPolyData> offsetMesh = TranslateMeshOrPoints<vtkPolyData>(mesh, off);

			meshOffsetPoints(mesh, boxData[0], boxData[1], boxData[2]);

			TreeNode* newNode = nullptr;
			// handle special case of only one tile
			if (lodLevelFiles.size() == 1 && opt.lodSplitfiles.size() == 1) {
				// no children at all, just a root node
				newNode = &_treeRoot;
			}
			else {
				newNode = new TreeNode;
				newNode->LOD = double(lodLevel);

				TreeNode* parentNode = (meshIndex < upperLevelNodes.size()) ? upperLevelNodes[meshIndex] : upperLevelNodes[0];

				newNode->parent = parentNode;
				parentNode->_children.push_back(newNode);

				nextLevelNodes.push_back(newNode);
			}

			//addBoundsInfo(crs, newNode, offsetMesh, meshObjFile, fileIndex, count);
			addBoundsInfo(newNode, mesh, meshObjFile, fileIndex, count);

			//ConvertDataSetCartesian(offsetMesh, crs.c_str());
			//ConvertDataSetCartesian(mesh, crs.c_str());
			ConvertDataSetCartesian2(mesh, crs.c_str());

#if 0
			auto tileMeshTcorrd = mesh->GetPointData()->GetTCoords();

			auto tCoords = mesh->GetPointData()->GetTCoords();
			offsetMesh->GetPointData()->SetTCoords(tCoords);
#endif

			//writeToGlb(offsetMesh, meshObjFile, opt.ouput3DTileFolder, fileIndex, count);
			writeToGlb(mesh, meshObjFile, opt.ouput3DTileFolder, fileIndex, count);
			++fileIndex;
			++meshIndex;
		}
		upperLevelNodes = nextLevelNodes;
		++lodLevel;
	}


	BoundsInfo fullBounds;
	auto VisitComputeBounds = [](TreeNode* node, void* aux)
		{
			BoundsInfo* fullBounds = (BoundsInfo*)aux;
			fullBounds->bb = ExpandBounds(fullBounds->bb.data(), node->info.bb.data());
		};
	PreOrderTraversal(VisitComputeBounds, &_treeRoot, &fullBounds);

	fullBounds.geoError = ComputeGeometricErrorTilesetMesh(fullBounds);
	_treeRoot.info.bb = fullBounds.bb;

	double lengthAux = 2.0 * fullBounds.geoError;
	auto VisitComputeGeometricError = [](TreeNode* node, void* aux)
		{
			if (node->IsLeaf())
			{
				node->info.geoError = 0.0;
			}
			else
			{
				node->info.geoError = ComputeGeometricErrorTilesetMesh(node->info) / node->LOD;
				// only true for octree or quadtree
				//double* parentError = static_cast<double*>(aux);
				//node->info.geoError = *parentError / 2.0;
			}
		};
	PreOrderTraversal(VisitComputeGeometricError, &_treeRoot, &lengthAux);


	// #ifdef INVERT_IT
	// 	auto VisitComputeGeometricErrorREVERSE = [](TreeNode* node, void* aux)
	// 			{
	// 				if (node->parent && node->parent->IsRoot()) {
	// 					node->info.geoError = 0.0;
	// 				}
	// 				else {
	// 					node->info.geoError = ComputeGeometricErrorTilesetMesh(node->info) / (3 - node->LOD+1);
	// 				}
	// 			};
	// 	PreOrderTraversal(VisitComputeGeometricErrorREVERSE, &_treeRoot, &lengthAux);
	// #endif



	std::string jsonFilename = opt.ouput3DTileFolder;
	jsonFilename += "\\tileset.json";

	//writeJson(jsonFilename, bounds);
	writeJson(&_treeRoot, jsonFilename, fullBounds.geoError);

	//OGRCoordinateTransformation::DestroyCT(coordTrans);

	deleteTree(&_treeRoot);

	// remove PNG files, they are not needed anymore
	std::vector<std::string> foundFilenamesPng;
	std::string folderToSearch = opt.ouput3DTileFolder;
	folderToSearch += "/";
	folderToSearch += TilesFolder;

#if 1
	bool foundFiles = StdUtility::findFiles(foundFilenamesPng, folderToSearch, ".*\\.png", true);
	for (auto& pngFile : foundFilenamesPng) {
		StdUtility::deleteFile(pngFile);
	}
#endif

	return true;
}


int doVTK_3DTiles(SlpkOptions& options)
{
	std::cout << "Starting 3DTiles" << std::endl;

	bool status = writeAs3DTiles(options);

	std::cout << "End 3DTiles" << std::endl;
	return 0;
}
