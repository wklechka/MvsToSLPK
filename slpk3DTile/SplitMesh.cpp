#include "SplitMesh.h"
#include "StdUtil\StdUtility.h"
#include "GuillotineBinPack.h"
#include "MaxRectsBinPack.h"

#include "happly/happly.h"

#include "i3s/i3s_writer.h"
#include "utils/utl_png.h"
#include "utils\utl_obb.h"
#include "i3s\i3s_common.h"

#include "vtkPNGWriter.h"
#include "vtkPNGReader.h"
#include "vtkNew.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkImageExtractComponents.h"

#include "vtkProcess.h"

#include "TexFunc.h"

// for operation that may or may not be thread safe
static std::mutex meshMutex;

bool writePNG_4to4(const std::string& pngFile, int w, int h, const char* buffer, int n_bytes)
{
	// assume 3 channel comming in...

	vtkNew<vtkImageData> oimage;
	oimage->SetDimensions(w, h, 1);
	oimage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);


	uint8_t* ptrScr = (uint8_t*)buffer;
	unsigned char* ipos = static_cast<unsigned char*>(oimage->GetScalarPointer());

	memcpy(ipos, ptrScr, n_bytes);


	vtkNew<vtkImageFlip> flip;
	flip->SetFilteredAxis(1);
	flip->SetInputData(oimage);

	vtkNew<vtkPNGWriter> pngWriter;
	pngWriter->SetInputConnection(flip->GetOutputPort());
	pngWriter->SetFileName(pngFile.c_str());
	pngWriter->Write();

	return true;
}






void set_texcoord(MyOpenMesh& mesh, OpenMesh::VertexHandle _vh, const OpenMesh::Vec2f& _texcoord)
{
	if (mesh.has_vertex_texcoords2D())
		mesh.set_texcoord2D(_vh, OpenMesh::vector_cast<MyOpenMesh::TexCoord2D>(_texcoord));
}
void set_texcoord(MyOpenMesh& mesh, OpenMesh::HalfedgeHandle _heh, const OpenMesh::Vec2f& _texcoord)
{
	if (mesh.has_halfedge_texcoords2D())
		mesh.set_texcoord2D(_heh, OpenMesh::vector_cast<MyOpenMesh::TexCoord2D>(_texcoord));
}

unsigned int get_face_texcoords(MyOpenMesh& mesh, std::vector<OpenMesh::Vec2f>& _hehandles)
{
	unsigned int count(0);
	_hehandles.clear();
	for (auto heh : mesh.halfedges().filtered(!OpenMesh::Predicates::Boundary()))
	{
		_hehandles.push_back(OpenMesh::vector_cast<OpenMesh::Vec2f>(mesh.texcoord2D(heh)));
		++count;
	}

	return count;
}

void add_face_texcoords(MyOpenMesh& mesh, OpenMesh::FaceHandle _fh, OpenMesh::VertexHandle _vh, const std::vector<OpenMesh::Vec2f>& _face_texcoords)
{
	// get first halfedge handle
	OpenMesh::HalfedgeHandle cur_heh = mesh.halfedge_handle(_fh);
	OpenMesh::HalfedgeHandle end_heh = mesh.prev_halfedge_handle(cur_heh);

	// find start heh
	while (mesh.to_vertex_handle(cur_heh) != _vh && cur_heh != end_heh)
		cur_heh = mesh.next_halfedge_handle(cur_heh);

	for (unsigned int i = 0; i < _face_texcoords.size(); ++i)
	{
		set_texcoord(mesh, cur_heh, _face_texcoords[i]);
		cur_heh = mesh.next_halfedge_handle(cur_heh);
	}
}

void MeshT::setFrom(MyOpenMesh& mesh, const std::string& ifname)
{
	_vertices.clear();
	_textureVertices.clear();
	_faces.clear();
	_materials.clear();

	// texture materials
	OpenMesh::MPropHandleT< std::map< int, std::string > > propertyTex;
	if (mesh.get_property_handle(propertyTex, "TextureMapping")) {
		auto props = mesh.property(propertyTex);

		std::string texFile;
		for (auto iter = props.begin(); iter != props.end(); ++iter) {
			if (!iter->second.empty()) {
				texFile = iter->second;
				break;
			}
		}

		Material newMaterial;
		newMaterial.textureFile = texFile;
		newMaterial.textureFileFullPath = StdUtility::getDirectory(ifname);
		newMaterial.textureFileFullPath += newMaterial.textureFile;
		_materials.push_back(newMaterial);
	}

	// Vertices

	int vCount = 0;
	for (MyOpenMesh::VertexIter vit = mesh.vertices_begin(); vit != mesh.vertices_end(); ++vit)
	{
		OpenMesh::Vec3f vert = mesh.point(*vit);
		_vertices.push_back(Vertex3(vert[0], vert[1], vert[2]));
		++vCount;
	}

	// Texture coordinate
	std::map<OpenMesh::Vec2f, int> texMap;
	std::vector<OpenMesh::Vec2f> texCoords;
	unsigned int num = get_face_texcoords(mesh, texCoords);
	for (size_t i = 0; i < num; ++i)
	{
		texMap[texCoords[i]] = static_cast<int>(i);
	}
	int texCount = 0;
	for (std::map<OpenMesh::Vec2f, int>::iterator it = texMap.begin(); it != texMap.end(); ++it)
	{
		it->second = texCount++;
	}

	int txCount = 0;
	for (std::map<OpenMesh::Vec2f, int>::iterator it = texMap.begin(); it != texMap.end(); ++it)
	{
		_textureVertices.push_back(Vertex2(it->first[0], it->first[1]));
		++txCount;
	}

	// Faces...

	int faceNum = 0;
	for (MyOpenMesh::FaceIter it = mesh.faces_begin(); it != mesh.faces_end(); ++it)
	{
		// first get the face vertex indexes 
		std::vector<int> faceVertexIndexes;
		for (auto vertIter = mesh.fv_ccwbegin(*it); vertIter != mesh.fv_ccwend(*it); ++vertIter) {
			faceVertexIndexes.push_back(vertIter->idx());
		}

		std::vector<int> faceTextureIndexes;
		for (auto hedgeIter = mesh.fh_ccwbegin(*it); hedgeIter != mesh.fh_ccwend(*it); ++hedgeIter) {

			const MyOpenMesh::TexCoord2D& tex = mesh.texcoord2D(*hedgeIter);

			OpenMesh::Vec2f txVal;
			txVal[0] = tex[0];
			txVal[1] = tex[1];

			auto mapIter = texMap.find(txVal);
			if (mapIter == texMap.end()) {
				// should never happen
			}
			else {
				faceTextureIndexes.push_back(mapIter->second);
			}

		}

		_faces.push_back(FaceT(faceVertexIndexes[0], faceVertexIndexes[1], faceVertexIndexes[2],
			faceTextureIndexes[0], faceTextureIndexes[1], faceTextureIndexes[2], 0));

		++faceNum;
	}
}

void MeshT::fillToOpenMesh(MyOpenMesh& mesh)
{
	removeUnused();

	mesh.resize(0, 0, 0);
	mesh.reserve(_vertices.size(), 3 * _vertices.size(), _faces.size());
	mesh.release_face_colors();

	// vertices
	for (auto& vert : _vertices) {
		OpenMesh::Vec3f newPt;
		newPt[0] = vert.X;
		newPt[1] = vert.Y;
		newPt[2] = vert.Z;
		mesh.add_vertex(newPt);
	}

	// faces
	int faceCount = 0;
	for (auto& theFace : _faces) {
		OpenMesh::IO::BaseImporter::VHandles vhandles;

		vhandles.push_back(OpenMesh::VertexHandle(theFace.IndexA));
		vhandles.push_back(OpenMesh::VertexHandle(theFace.IndexB));
		vhandles.push_back(OpenMesh::VertexHandle(theFace.IndexC));

		if (faceCount == 64) {
			printf("");
		}

		OpenMesh::FaceHandle fh = mesh.add_face(vhandles);

		if (fh.idx() < 0) {
			continue;
		}

		std::vector<OpenMesh::Vec2f> texValues;
		OpenMesh::Vec2f tmpTx;

		tmpTx[0] = _textureVertices[theFace.TextureIndexA].X;
		tmpTx[1] = _textureVertices[theFace.TextureIndexA].Y;
		texValues.push_back(tmpTx);
		tmpTx[0] = _textureVertices[theFace.TextureIndexB].X;
		tmpTx[1] = _textureVertices[theFace.TextureIndexB].Y;
		texValues.push_back(tmpTx);
		tmpTx[0] = _textureVertices[theFace.TextureIndexC].X;
		tmpTx[1] = _textureVertices[theFace.TextureIndexC].Y;
		texValues.push_back(tmpTx);

		add_face_texcoords(mesh, fh, vhandles[0], texValues);
		++faceCount;
	}

	mesh.release_face_colors();
}

bool MeshT::writeToObj(const std::string objFilename)
{
	MyOpenMesh mesh;
	fillToOpenMesh(mesh);

	// binary mode is not supported for OBJ
	//OpenMesh::IO::Options wopt = OpenMesh::IO::Options::Flag::FaceTexCoord | OpenMesh::IO::Options::Flag::Binary;
	OpenMesh::IO::Options wopt = OpenMesh::IO::Options::Flag::FaceTexCoord;

	if (_materials.size() > 0) {
		wopt.texture_file = _materials[0].textureFile;
	}

	wopt.material_file_extension = ".obj.mtl";

	//meshMutex.lock();
	mesh.release_face_colors();
	bool status = OpenMesh::IO::write_mesh(mesh, objFilename, wopt);
	//meshMutex.unlock();

	return status;
}

int MeshT::splitMesh(IVertexUtilsBase* utils, double q, MeshT& left, MeshT& right)
{
	int count = 0;

	IVertexUtilsBase& utilsRef = *utils;

	MapDict<Vertex3, int> leftVertices;
	MapDict<Vertex3, int> rightVertices;

	LIST_Type<FaceT> leftFaces;
	LIST_Type<FaceT> rightFaces;

	MapDict<Vertex2, int> leftTextureVertices;
	MapDict<Vertex2, int> rightTextureVertices;

	for (int index = 0; index < _faces.size(); index++)
	{
		auto face = _faces[index];

		auto vA = _vertices[face.IndexA];
		auto vB = _vertices[face.IndexB];
		auto vC = _vertices[face.IndexC];

		auto vtA = _textureVertices[face.TextureIndexA];
		auto vtB = _textureVertices[face.TextureIndexB];
		auto vtC = _textureVertices[face.TextureIndexC];

		auto aSide = utilsRef.getDim(vA) < q;
		auto bSide = utilsRef.getDim(vB) < q;
		auto cSide = utilsRef.getDim(vC) < q;

		if (aSide)
		{
			if (bSide)
			{
				if (cSide)
				{
					// All on the left

					auto indexALeft = addIndex(leftVertices, vA);
					auto indexBLeft = addIndex(leftVertices, vB);
					auto indexCLeft = addIndex(leftVertices, vC);

					auto indexATextureLeft = addIndex(leftTextureVertices, vtA);
					auto indexBTextureLeft = addIndex(leftTextureVertices, vtB);
					auto indexCTextureLeft = addIndex(leftTextureVertices, vtC);

					leftFaces.push_back(FaceT(indexALeft, indexBLeft, indexCLeft,
						indexATextureLeft, indexBTextureLeft, indexCTextureLeft,
						face.MaterialIndex));
				}
				else
				{
					addCutTriRight2D(utils, q, face.IndexC, face.IndexA, face.IndexB,
						leftVertices,
						rightVertices,
						face.TextureIndexC, face.TextureIndexA, face.TextureIndexB,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces, rightFaces
					);
					count++;
				}
			}
			else
			{
				if (cSide)
				{
					addCutTriRight2D(utils, q, face.IndexB, face.IndexC, face.IndexA,
						leftVertices,
						rightVertices,
						face.TextureIndexB, face.TextureIndexC, face.TextureIndexA,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces,
						rightFaces);
					count++;
				}
				else
				{
					addCutTriLeft2D(utils, q, face.IndexA, face.IndexB, face.IndexC,
						leftVertices,
						rightVertices,
						face.TextureIndexA, face.TextureIndexB, face.TextureIndexC,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces,
						rightFaces);
					count++;
				}
			}
		}
		else
		{
			if (bSide)
			{
				if (cSide)
				{
					addCutTriRight2D(utils, q, face.IndexA, face.IndexB, face.IndexC,
						leftVertices,
						rightVertices,
						face.TextureIndexA, face.TextureIndexB, face.TextureIndexC,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces,
						rightFaces);
					count++;
				}
				else
				{
					addCutTriLeft2D(utils, q, face.IndexB, face.IndexC, face.IndexA,
						leftVertices,
						rightVertices,
						face.TextureIndexB, face.TextureIndexC, face.TextureIndexA,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces,
						rightFaces);
					count++;
				}
			}
			else
			{
				if (cSide)
				{
					addCutTriLeft2D(utils, q, face.IndexC, face.IndexA, face.IndexB,
						leftVertices,
						rightVertices,
						face.TextureIndexC, face.TextureIndexA, face.TextureIndexB,
						leftTextureVertices, rightTextureVertices, face.MaterialIndex, leftFaces,
						rightFaces);
					count++;
				}
				else
				{
					// All on the right

					auto indexARight = addIndex(rightVertices, vA);
					auto indexBRight = addIndex(rightVertices, vB);
					auto indexCRight = addIndex(rightVertices, vC);

					auto indexATextureRight = addIndex(rightTextureVertices, vtA);
					auto indexBTextureRight = addIndex(rightTextureVertices, vtB);
					auto indexCTextureRight = addIndex(rightTextureVertices, vtC);

					rightFaces.push_back(FaceT(indexARight, indexBRight, indexCRight,
						indexATextureRight, indexBTextureRight, indexCTextureRight,
						face.MaterialIndex));
				}
			}
		}
	}

	auto orderedLeftVertices = orderByVal(leftVertices);
	auto orderedRightVertices = orderByVal(rightVertices);
	auto rightMaterials = _materials;

	auto orderedLeftTextureVertices = orderByVal(leftTextureVertices);
	auto orderedRightTextureVertices = orderByVal(rightTextureVertices);

	auto leftMaterials = _materials;

	left.set(orderedLeftVertices, orderedLeftTextureVertices, leftFaces, leftMaterials);

	right.set(orderedRightVertices, orderedRightTextureVertices, rightFaces, rightMaterials);

	return count;
}

static double getIntersectPercent(Vertex3 a, Vertex3 b, Vertex3 p)
{
	auto edge1Length = a.Distance(b);
	auto subEdge1Length = a.Distance(p);
	return subEdge1Length / edge1Length;
}

void MeshT::addCutTriLeft2D(IVertexUtilsBase* utilsBase, double q, int indexVL,
	int indexVR1, int indexVR2,
	MapDict<Vertex3, int>& leftVertices, MapDict<Vertex3, int>& rightVertices,
	int indexTextureVL, int indexTextureVR1, int indexTextureVR2,
	MapDict<Vertex2, int>& leftTextureVertices, MapDict<Vertex2, int>& rightTextureVertices,
	int materialIndex, LIST_Type<FaceT>& leftFaces, LIST_Type<FaceT>& rightFaces)
{
	IVertexUtilsBase& utils = *utilsBase;


	auto vL = _vertices[indexVL];
	auto vR1 = _vertices[indexVR1];
	auto vR2 = _vertices[indexVR2];

	auto tVL = _textureVertices[indexTextureVL];
	auto tVR1 = _textureVertices[indexTextureVR1];
	auto tVR2 = _textureVertices[indexTextureVR2];

	auto indexVLLeft = addIndex(leftVertices, vL);
	auto indexTextureVLLeft = addIndex(leftTextureVertices, tVL);

	if (fabs(utils.getDim(vR1) - q) < Epsilon &&
		fabs(utils.getDim(vR2) - q) < Epsilon)
	{
		// Right Vertices are on the line

		auto indexVR1Left = addIndex(leftVertices, vR1);
		auto indexVR2Left = addIndex(leftVertices, vR2);

		auto indexTextureVR1Left = addIndex(leftTextureVertices, tVR1);
		auto indexTextureVR2Left = addIndex(leftTextureVertices, tVR2);

		leftFaces.push_back(FaceT(indexVLLeft, indexVR1Left, indexVR2Left,
			indexTextureVLLeft, indexTextureVR1Left, indexTextureVR2Left, materialIndex));

		return;
	}

	auto indexVR1Right = addIndex(rightVertices, vR1);
	auto indexVR2Right = addIndex(rightVertices, vR2);

	// a on the left, b and c on the right

	// first intersection
	auto t1 = utils.cutEdge(vL, vR1, q);
	auto indexT1Left = addIndex(leftVertices, t1);
	auto indexT1Right = addIndex(rightVertices, t1);

	// second intersection
	auto t2 = utils.cutEdge(vL, vR2, q);
	auto indexT2Left = addIndex(leftVertices, t2);
	auto indexT2Right = addIndex(rightVertices, t2);

	// Split texture
	auto indexTextureVR1Right = addIndex(rightTextureVertices, tVR1);
	auto indexTextureVR2Right = addIndex(rightTextureVertices, tVR2);

	auto perc1 = getIntersectPercent(vL, vR1, t1);

	// First intersection texture
	auto t1t = tVL.cutEdgePerc(tVR1, perc1);
	auto indexTextureT1Left = addIndex(leftTextureVertices, t1t);
	auto indexTextureT1Right = addIndex(rightTextureVertices, t1t);

	auto perc2 = getIntersectPercent(vL, vR2, t2);

	// Second intersection texture
	auto t2t = tVL.cutEdgePerc(tVR2, perc2);
	auto indexTextureT2Left = addIndex(leftTextureVertices, t2t);
	auto indexTextureT2Right = addIndex(rightTextureVertices, t2t);

	auto lface = FaceT(indexVLLeft, indexT1Left, indexT2Left,
		indexTextureVLLeft, indexTextureT1Left, indexTextureT2Left, materialIndex);
	leftFaces.push_back(lface);

	auto rface1 = FaceT(indexT1Right, indexVR1Right, indexVR2Right,
		indexTextureT1Right, indexTextureVR1Right, indexTextureVR2Right, materialIndex);
	rightFaces.push_back(rface1);

	auto rface2 = FaceT(indexT1Right, indexVR2Right, indexT2Right,
		indexTextureT1Right, indexTextureVR2Right, indexTextureT2Right, materialIndex);
	rightFaces.push_back(rface2);
}

void MeshT::addCutTriRight2D(IVertexUtilsBase* utilsBase, double q, int indexVR,
	int indexVL1, int indexVL2,
	MapDict<Vertex3, int>& leftVertices, MapDict<Vertex3, int>& rightVertices,
	int indexTextureVR, int indexTextureVL1, int indexTextureVL2,
	MapDict<Vertex2, int>& leftTextureVertices, MapDict<Vertex2, int>& rightTextureVertices,
	int materialIndex, LIST_Type<FaceT>& leftFaces, LIST_Type<FaceT>& rightFaces)
{
	IVertexUtilsBase& utils = *utilsBase;

	auto vR = _vertices[indexVR];
	auto vL1 = _vertices[indexVL1];
	auto vL2 = _vertices[indexVL2];

	auto tVR = _textureVertices[indexTextureVR];
	auto tVL1 = _textureVertices[indexTextureVL1];
	auto tVL2 = _textureVertices[indexTextureVL2];

	auto indexVRRight = addIndex(rightVertices, vR);
	auto indexTextureVRRight = addIndex(rightTextureVertices, tVR);

	if (fabs(utils.getDim(vL1) - q) < Epsilon &&
		fabs(utils.getDim(vL2) - q) < Epsilon)
	{
		// Left Vertices are on the line

		auto indexVL1Right = addIndex(rightVertices, vL1);
		auto indexVL2Right = addIndex(rightVertices, vL2);

		auto indexTextureVL1Right = addIndex(rightTextureVertices, tVL1);
		auto indexTextureVL2Right = addIndex(rightTextureVertices, tVL2);

		rightFaces.push_back(FaceT(indexVRRight, indexVL1Right, indexVL2Right,
			indexTextureVRRight, indexTextureVL1Right, indexTextureVL2Right, materialIndex));

		return;
	}

	auto indexVL1Left = addIndex(leftVertices, vL1);
	auto indexVL2Left = addIndex(leftVertices, vL2);

	// a on the right, b and c on the left

	// frist intersection
	auto t1 = utils.cutEdge(vR, vL1, q);
	auto indexT1Left = addIndex(leftVertices, t1);
	auto indexT1Right = addIndex(rightVertices, t1);

	// Second intersection
	auto t2 = utils.cutEdge(vR, vL2, q);
	auto indexT2Left = addIndex(leftVertices, t2);
	auto indexT2Right = addIndex(rightVertices, t2);

	// Split texture
	auto indexTextureVL1Left = addIndex(leftTextureVertices, tVL1);
	auto indexTextureVL2Left = addIndex(leftTextureVertices, tVL2);

	auto perc1 = getIntersectPercent(vR, vL1, t1);

	// first intersection texture
	auto t1t = tVR.cutEdgePerc(tVL1, perc1);
	auto indexTextureT1Left = addIndex(leftTextureVertices, t1t);
	auto indexTextureT1Right = addIndex(rightTextureVertices, t1t);

	auto perc2 = getIntersectPercent(vR, vL2, t2);

	// Second intersection texture
	auto t2t = tVR.cutEdgePerc(tVL2, perc2);
	auto indexTextureT2Left = addIndex(leftTextureVertices, t2t);
	auto indexTextureT2Right = addIndex(rightTextureVertices, t2t);

	auto rface = FaceT(indexVRRight, indexT1Right, indexT2Right,
		indexTextureVRRight, indexTextureT1Right, indexTextureT2Right, materialIndex);
	rightFaces.push_back(rface);

	auto lface1 = FaceT(indexT2Left, indexVL1Left, indexVL2Left,
		indexTextureT2Left, indexTextureVL1Left, indexTextureVL2Left, materialIndex);
	leftFaces.push_back(lface1);

	auto lface2 = FaceT(indexT2Left, indexT1Left, indexVL1Left,
		indexTextureT2Left, indexTextureT1Left, indexTextureVL1Left, materialIndex);
	leftFaces.push_back(lface2);
}

Box3 MeshT::calcBounds()
{
	auto minX = MaxValue;
	auto minY = MaxValue;
	auto minZ = MaxValue;

	auto maxX = MinValue;
	auto maxY = MinValue;
	auto maxZ = MinValue;

	for (auto index = 0; index < _vertices.size(); index++)
	{
		auto v = _vertices[index];
		minX = minX < v.X ? minX : v.X;
		minY = minY < v.Y ? minY : v.Y;
		minZ = minZ < v.Z ? minZ : v.Z;

		maxX = v.X < maxX ? maxX : v.X;
		maxY = v.Y < maxY ? maxY : v.Y;
		maxZ = v.Z < maxZ ? maxZ : v.Z;
	}

	_bounds = Box3(minX, minY, minZ, maxX, maxY, maxZ);
	return _bounds;
}
Vertex3 MeshT::baricenter()
{

	auto x = 0.0;
	auto y = 0.0;
	auto z = 0.0;

	for (auto index = 0; index < _vertices.size(); index++)
	{
		auto v = _vertices[index];
		x += v.X;
		y += v.Y;
		z += v.Z;
	}

	x /= _vertices.size();
	y /= _vertices.size();
	z /= _vertices.size();

	return Vertex3(x, y, z);
}

MeshT::Rectangle MeshT::getClusterRect(LIST_Type<int>& cluster)
{
	double maxX = MinValue, maxY = MinValue;
	double minX = MaxValue, minY = MaxValue;

	for (auto n = 0; n < cluster.size(); n++)
	{
		auto face = _faces[cluster[n]];

		auto vtA = _textureVertices[face.TextureIndexA];
		auto vtB = _textureVertices[face.TextureIndexB];
		auto vtC = _textureVertices[face.TextureIndexC];

		maxX = std::max(std::max(std::max(maxX, vtC.X), vtB.X), vtA.X);
		maxY = std::max(std::max(std::max(maxY, vtC.Y), vtB.Y), vtA.Y);

		minX = std::min(std::min(std::min(minX, vtC.X), vtB.X), vtA.X);
		minY = std::min(std::min(std::min(minY, vtC.Y), vtB.Y), vtA.Y);
	}

	return Rectangle(minX, minY, (maxX - minX), (maxY - minY));
}

void MeshT::calculateMaxMinAreaRect(std::vector<MeshT::Rectangle>& clustersRects, int textureWidth, int textureHeight,
	double& maxWidth, double& maxHeight, double& textureArea)
{
	maxWidth = 0;
	maxHeight = 0;
	textureArea = 0;

	for (auto index = 0; index < clustersRects.size(); index++)
	{
		auto rect = clustersRects[index];

		textureArea += std::max(ceil(rect.Width * textureWidth), 1.0) *
			std::max(ceil(rect.Height * textureHeight), 1.0);

		if (rect.Width > maxWidth)
		{
			maxWidth = rect.Width;
		}

		if (rect.Height > maxHeight)
		{
			maxHeight = rect.Height;
		}
	}

	maxWidth = ceil(maxWidth * textureWidth);
	maxHeight = ceil(maxHeight * textureHeight);
}

void MeshT::removeUnused()
{
	MapDict<Vertex3, int> newVertexes;
	MapDict<Vertex2, int> newUvs;

	//auto newMaterials = new Dictionary<Material, int>(_materials.Count);

	for (auto f = 0; f < _faces.size(); f++)
	{
		auto& face = _faces[f];

		// Vertices

		auto vA = _vertices[face.IndexA];
		auto vB = _vertices[face.IndexB];
		auto vC = _vertices[face.IndexC];

		face.IndexA = addIndex(newVertexes, vA);
		face.IndexB = addIndex(newVertexes, vB);
		face.IndexC = addIndex(newVertexes, vC);

		// Texture vertices

		auto uvA = _textureVertices[face.TextureIndexA];
		auto uvB = _textureVertices[face.TextureIndexB];
		auto uvC = _textureVertices[face.TextureIndexC];

		face.TextureIndexA = addIndex(newUvs, uvA);
		face.TextureIndexB = addIndex(newUvs, uvB);
		face.TextureIndexC = addIndex(newUvs, uvC);


		// Materials
	}

	_vertices = orderByVal(newVertexes);
	_textureVertices = orderByVal(newUvs);

}

std::vector<std::vector<int>> MeshT::getFacesByMaterial()
{
	std::vector<int> faceIndex;

	for (auto i = 0; i < _faces.size(); i++)
	{
		faceIndex.push_back(i);
	}

	std::vector<std::vector<int>> facesPerMaterial;
	facesPerMaterial.push_back(faceIndex);
	return facesPerMaterial;
}

LIST_Type<LIST_Type<int>> getFacesClusters(LIST_Type<int> &facesIndexes,
	MapDict<int, LIST_Type<int>> &facesMapper)
{
	LIST_Type<LIST_Type<int>>  clusters;
	LIST_Type<int> remainingFacesIndexes = facesIndexes;
	

	LIST_Type<int> currentCluster;
	currentCluster.push_back(remainingFacesIndexes[0]);
	
	HashSet<int> currentClusterCache;
	currentClusterCache.insert(remainingFacesIndexes[0]);

	remainingFacesIndexes.erase(remainingFacesIndexes.begin());

	auto lastRemainingFacesCount = remainingFacesIndexes.size();

	while (remainingFacesIndexes.size() > 0)
	{
		auto cnt = currentCluster.size();

		for (auto index = 0; index < currentCluster.size(); index++)
		{
			auto faceIndex = currentCluster[index];

			if (facesMapper.find(faceIndex) == facesMapper.end()) {
				continue;
			}

			auto& connectedFaces = facesMapper[faceIndex];

			for (auto i = 0; i < connectedFaces.size(); i++)
			{
				auto connectedFace = connectedFaces[i];

				if (currentClusterCache.find(connectedFace) != currentClusterCache.end()) {
					continue;
				}

				currentCluster.push_back(connectedFace);
				currentClusterCache.insert(connectedFace);

				auto iter = std::find(remainingFacesIndexes.begin(), remainingFacesIndexes.end(), connectedFace);
				remainingFacesIndexes.erase(iter);
			}
		}

		// No new face was added
		if (cnt == currentCluster.size())
		{
			clusters.push_back(currentCluster);

			if (remainingFacesIndexes.size() == 0) 
				break;

			// continue with the next cluster

			currentCluster.clear();
			currentCluster.push_back(remainingFacesIndexes[0]);

			currentClusterCache.clear();
			currentClusterCache.insert(remainingFacesIndexes[0]);


			remainingFacesIndexes.erase(remainingFacesIndexes.begin());
		}

		if (lastRemainingFacesCount == remainingFacesIndexes.size())
		{
			break;
		}

		lastRemainingFacesCount = remainingFacesIndexes.size();
	}

	clusters.push_back(currentCluster);

	return clusters;
}


MapDict<int, LIST_Type<int>> MeshT::getFacesMapper(MapDict<MeshT::Edge, LIST_Type<int>> &edgesMapper)
{
	MapDict<int, LIST_Type<int>> facesMapper;

	for (auto iter = edgesMapper.begin(); iter != edgesMapper.end(); ++iter) {
		
		LIST_Type<int>& Value = iter->second;

		for (auto i = 0; i < Value.size(); ++i) {
			auto faceIndex = Value[i];

			if (facesMapper.find(faceIndex) == facesMapper.end()) {
				LIST_Type<int> newList;
				newList.push_back(faceIndex);
				facesMapper[faceIndex] = newList;
			}
				

			for (auto index = 0; index < Value.size(); index++)
			{
				auto f = Value[index];
				if (f != faceIndex)
					facesMapper[faceIndex].push_back(f);
			}
		}
	}

	return facesMapper;
}

MapDict<MeshT::Edge, LIST_Type<int>> MeshT::getEdgesMapper(LIST_Type<int> &facesIndexes)
{
	MapDict<Edge, LIST_Type<int>> edgesMapper;

	for (auto idx = 0; idx < facesIndexes.size(); idx++)
	{
		auto faceIndex = facesIndexes[idx];
		auto &f = _faces[faceIndex];

		auto e1 = Edge(f.TextureIndexA, f.TextureIndexB);
		auto e2 = Edge(f.TextureIndexB, f.TextureIndexC);
		auto e3 = Edge(f.TextureIndexA, f.TextureIndexC);

		if (edgesMapper.find(e1) == edgesMapper.end()) {
			edgesMapper[e1] = LIST_Type<int>();
		}
		if (edgesMapper.find(e2) == edgesMapper.end()) {
			edgesMapper[e2] = LIST_Type<int>();
		}
		if (edgesMapper.find(e3) == edgesMapper.end()) {
			edgesMapper[e3] = LIST_Type<int>();
		}

		// Will - added max number of edges
		// degen mesh had bunches going to same texture coord
		// this could easily be less than 100
		// without this limit massive amount of mem use 50GB for div 1
		// forcing use of div 3 and still 10GB
		const int maxEdge = 100;

		if (edgesMapper[e1].size() < maxEdge)
			edgesMapper[e1].push_back(faceIndex);
		if (edgesMapper[e3].size() < maxEdge)
			edgesMapper[e2].push_back(faceIndex);
		if (edgesMapper[e3].size() < maxEdge)
			edgesMapper[e3].push_back(faceIndex);
	}

	return edgesMapper;
}


// not working well...so

void MeshT::regenerateTexturesCopy(const std::string& targetFolder, const std::string& objFilename)
{
	// don't rengen just copy and use the same texture for each split
	auto& material = _materials[0];

	if (!StdUtility::fileExists(material.textureFileFullPath)) {
		return ;
	}

	std::string newTexFilename = targetFolder;
	newTexFilename += "\\";
	newTexFilename += StdUtility::getName(objFilename);
	newTexFilename += ".png";


	std::vector<uint8_t> texture;
	int width, height;
	if (!readPNG(material.textureFileFullPath, texture, width, height)) {
		return;
	}

	if (width <= 512) {
		StdUtility::copyFile(material.textureFileFullPath, newTexFilename);
	}
	else {
		std::vector<std::vector<uint8_t>> textures;

		std::vector<int> texturesSizes;

		uint8_t* pixelData = texture.data();
		build_downsampled_textures(width, pixelData, 256, textures, texturesSizes);

		int depth = 1;
		auto& texture = (depth < textures.size()) ? textures[depth] : textures.back();
		auto& textureSize = (depth < texturesSizes.size()) ? texturesSizes[depth] : texturesSizes.back();

		writePNG(newTexFilename, textureSize, textureSize, (const char*)texture.data(), texture.size());
	}	
}

void MeshT::regenerateTextures(const std::string& targetFolder, const std::string& objFilename)
{
	// for now just all faces
	auto facesByMaterial = getFacesByMaterial();

	for (auto m = 0; m < facesByMaterial.size(); m++)
	{
		auto material = _materials[m];
		auto facesIndexes = facesByMaterial[m];
		

		if (facesIndexes.size() == 0)
		{
			continue;
		}

		auto edgesMapper = getEdgesMapper(facesIndexes);
		auto facesMapper = getFacesMapper(edgesMapper);
		auto clusters = getFacesClusters(facesIndexes, facesMapper);

		auto byCount = [](LIST_Type<int> &L1, LIST_Type<int> &L2)
			{
				return L1.size() > L2.size();
			};
		std::sort(clusters.begin(), clusters.end(), byCount);
		
		binPackTextures(targetFolder, m, clusters, objFilename);
	}
}


class TextureImage
{
public:
	TextureImage()
	{}

	TextureImage(int w, int h)
	{
		resize(w, h);
	}

	void resize(int w, int h)
	{
		_width = w;
		_height = h;
		_pixels.resize(w * h * numChannel);
		std::fill(_pixels.begin(), _pixels.end(), 0);
	}

private:
	std::vector<uint8_t> _pixels;
	int _width;
	int _height;
	int numChannel = 4;

public:
	int width() const { return _width;}
	int height() const { return _height; }

	bool loadTexture(const std::string& textureFile)
	{
		return readPNG(textureFile, _pixels, _width, _height);
	}
	bool saveTexture(const std::string& textureFile)
	{
		return writePNG_4to4(textureFile, _width, _height, (const char*)_pixels.data(), _pixels.size());
	}

	static void copyImageRect(TextureImage &sourceImage, TextureImage &dest, int sourceX, int sourceY, int sourceWidth, int sourceHeight, int destX, int destY)
	{
		int sourceWidthBytes = sourceImage._width * sourceImage.numChannel;
		int destWidthBytes = dest._width * dest.numChannel;

		uint8_t* startPos = sourceImage._pixels.data() + (sourceX * sourceImage.numChannel) + sourceY * (sourceWidthBytes);
		uint8_t* destPos = dest._pixels.data() + (destX * dest.numChannel) + destY * (destWidthBytes);

		int widthToCopy = sourceWidth * sourceImage.numChannel;

		for (int i = 0; i < sourceHeight; i++) {
			memcpy(destPos, startPos, widthToCopy);
			destPos += destWidthBytes;
			startPos += sourceWidthBytes;
		}
	}

private:
	bool readPNG(const std::string& textureFile, std::vector<uint8_t>& data, int& width, int& height)
	{
		std::vector<char>& dataRef = *((std::vector<char>*) & data); // had to cast it
		if (!i3slib::utl::read_png_from_file(textureFile, &width, &height, &dataRef))
		{
			std::cout << "Failed to load color bitmap from file." << std::endl;
			return false;
		}
		return true;
	}
	

};

static int nextPowerOfTwo(int x)
{
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return (x + 1);
}
static double clamp(double value, double min, double max)
{
	if (min > max)
	{
		std::swap(min, max);
	}

	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}

	return value;
}


bool MeshT::binPackTextures(const std::string &targetFolder, int materialIndex, LIST_Type<LIST_Type<int>> &clusters, const std::string& objFilename)
{
	auto &material = _materials[materialIndex];

	if (!StdUtility::fileExists(material.textureFileFullPath)) {
		return false;
	}

	TextureImage texture;
	texture.loadTexture(material.textureFileFullPath);

	auto textureWidth = texture.width();
	auto textureHeight = texture.height();

	LIST_Type<Rectangle> clustersRects;
	for (auto& cluster : clusters) {
		clustersRects.push_back(getClusterRect(cluster));
	}
	

	double maxWidth; double maxHeight; double textureArea;
	calculateMaxMinAreaRect(clustersRects, textureWidth, textureHeight, maxWidth, maxHeight, textureArea);

	auto edgeLength = std::max(nextPowerOfTwo((int)sqrt(textureArea)), 32);

	if (edgeLength < maxWidth)
		edgeLength = nextPowerOfTwo((int)maxWidth);

	if (edgeLength < maxHeight)
		edgeLength = nextPowerOfTwo((int)maxHeight);



	rbp::MaxRectsBinPack binPack(edgeLength, edgeLength, false);


	TextureImage newTexture(edgeLength, edgeLength);

	auto count = 0;

	MapDict<Vertex2, int> newTextureVertices;

	auto facesCopy = _faces;

	for (auto i = 0; i < clusters.size(); i++)
	{
		auto cluster = clusters[i];
		auto clusterBoundary = clustersRects[i];

		auto clusterX = (int)floor(clusterBoundary.X * (textureWidth - 1));
		auto clusterY = (int)floor(clusterBoundary.Y * (textureHeight - 1));
		auto clusterWidth = (int)std::max(ceil(clusterBoundary.Width * textureWidth), 1.0);
		auto clusterHeight = (int)std::max(ceil(clusterBoundary.Height * textureHeight), 1.0);

		rbp::Rect newTextureClusterRect = binPack.Insert(clusterWidth, clusterHeight, rbp::MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestAreaFit);

		if (newTextureClusterRect.width == 0)
		{
			// need to do a total do over cause the boxes don't fit and we don't want to make another texture

			std::cout << "Texture Restarted" << std::endl;

			// restart loop - sort of evil

			edgeLength *= 2; // make it bigger so it will fit
			
			i = -1;  // restart the loop

			// set things back the way they were
			newTexture.resize(edgeLength, edgeLength);
			newTextureVertices.clear();
			binPack.Init(edgeLength, edgeLength, false);

			facesCopy = _faces;

			continue;
		}

		auto adjustedSourceY = std::max(texture.height() - (clusterY + clusterHeight), 1);
		auto adjustedDestY = std::max(edgeLength - (newTextureClusterRect.y + clusterHeight), 0);
		
		TextureImage::copyImageRect(texture, newTexture, clusterX, adjustedSourceY, clusterWidth, clusterHeight,
			newTextureClusterRect.x, adjustedDestY);

		auto textureScaleX = (double)textureWidth / edgeLength;
		auto textureScaleY = (double)textureHeight / edgeLength;

		for (auto index = 0; index < cluster.size(); index++)
		{
			auto faceIndex = cluster[index];

			//auto &face = _faces[faceIndex];
			auto& face = facesCopy[faceIndex];
			

			auto vtA = _textureVertices[face.TextureIndexA];
			auto vtB = _textureVertices[face.TextureIndexB];
			auto vtC = _textureVertices[face.TextureIndexC];

			// Translation relative to the cluster (percentage)
			auto vtAdx = std::max(0.0, vtA.X - clusterBoundary.X) * textureScaleX;
			auto vtAdy = std::max(0.0, vtA.Y - clusterBoundary.Y) * textureScaleY;

			auto vtBdx = std::max(0.0, vtB.X - clusterBoundary.X) * textureScaleX;
			auto vtBdy = std::max(0.0, vtB.Y - clusterBoundary.Y) * textureScaleY;

			auto vtCdx = std::max(0.0, vtC.X - clusterBoundary.X) * textureScaleX;
			auto vtCdy = std::max(0.0, vtC.Y - clusterBoundary.Y) * textureScaleY;

			// Cluster relative positions (percentage)
			auto relativeClusterX = newTextureClusterRect.x / (double)edgeLength;
			auto relativeClusterY = newTextureClusterRect.y / (double)edgeLength;


			auto newVtA = Vertex2(clamp(relativeClusterX + vtAdx, 0.0, 1.0),
				clamp(relativeClusterY + vtAdy, 0.0, 1.0));
			auto newVtB = Vertex2(clamp(relativeClusterX + vtBdx, 0.0, 1.0),
				clamp(relativeClusterY + vtBdy, 0.0, 1.0));
			auto newVtC = Vertex2(clamp(relativeClusterX + vtCdx, 0.0, 1.0),
				clamp(relativeClusterY + vtCdy, 0.0, 1.0));

			auto newIndexVtA = addIndex(newTextureVertices, newVtA);
			auto newIndexVtB = addIndex(newTextureVertices, newVtB);
			auto newIndexVtC = addIndex(newTextureVertices, newVtC);

			face.TextureIndexA = newIndexVtA;
			face.TextureIndexB = newIndexVtB;
			face.TextureIndexC = newIndexVtC;
			face.MaterialIndex = materialIndex;
		}
	}

	// set the actual mesh faces;
	_faces = facesCopy;
	// set the new texture coordinate
	_textureVertices = orderByVal(newTextureVertices);

	std::string newTexFilename = targetFolder;
	newTexFilename += "\\";
	newTexFilename += StdUtility::getName(objFilename);
	newTexFilename += ".png";

	//meshMutex.lock();
	newTexture.saveTexture(newTexFilename);
	//meshMutex.unlock();

	_materials[0].textureFileFullPath = newTexFilename;
	_materials[0].textureFile = StdUtility::getName(newTexFilename, false);

	return true;
}



template <typename T>
void addFaceIndices(happly::PLYData& plyOut, std::vector<std::vector<T>>& indices, std::vector<std::vector<float>>& meshTexCoords) {

	std::string faceName = "face";
	size_t N = indices.size();

	// Create the element
	if (!plyOut.hasElement(faceName)) {
		plyOut.addElement(faceName, N);
	}

	// Cast to 32 bit
	typedef typename std::conditional<std::is_signed<T>::value, int32_t, uint32_t>::type IndType;
	std::vector<std::vector<IndType>> intInds;
	for (std::vector<T>& l : indices) {
		std::vector<IndType> thisInds;
		for (T& val : l) {
			IndType valConverted = static_cast<IndType>(val);
			if (valConverted != val) {
				throw std::runtime_error("Index value " + std::to_string(val) +
					" could not be converted to a .ply integer without loss of data. Note that .ply "
					"only supports 32-bit ints.");
			}
			thisInds.push_back(valConverted);
		}
		intInds.push_back(thisInds);
	}

	// Store
	plyOut.getElement(faceName).addListProperty<IndType>("vertex_indices", intInds);

	plyOut.getElement(faceName).addListProperty<float>("texcoord", meshTexCoords);
}

// This works great for writing PLY
// OpenMesh fails on PLY with textures!
// just use this...
class PointMesh
{
public:
	double x, y, z;

	uint32_t color;

// 	double tx = 0.0;
// 	double ty = 0.0;
};

class FaceIt
{
public:
	int index[3];
	double tx[3];
	double ty[3];
};

static bool makeMeshPLYTextures(std::vector<PointMesh>& pts, std::vector<FaceIt>& faces, const std::string& filename)
{
	// Suppose these hold your data
	std::vector<std::array<double, 3>> meshVertexPositions;
	//std::vector<std::array<double, 3>> meshVertexColors;
	std::vector<std::vector<size_t>> meshFaceIndices;
	std::vector<std::vector<float>> meshTexCoords;

	for (auto& pt : pts) {
		std::array<double, 3> ptArr;
		ptArr[0] = pt.x;
		ptArr[1] = pt.y;
		ptArr[2] = pt.z;
		meshVertexPositions.push_back(ptArr);
	}

	for (int i = 0; i < faces.size(); ++i) {

		int index1 = faces[i].index[0];
		int index2 = faces[i].index[1];
		int index3 = faces[i].index[2];

		std::vector<size_t> faceIndex;
		faceIndex.push_back(index1);
		faceIndex.push_back(index2);
		faceIndex.push_back(index3);

		meshFaceIndices.push_back(faceIndex);

		std::vector<float> tx1;
#pragma warning(push)
#pragma warning(disable: 4244)
		tx1.push_back(faces[i].tx[0]);
		tx1.push_back(faces[i].ty[0]);

		tx1.push_back(faces[i].tx[1]);
		tx1.push_back(faces[i].ty[1]);

		tx1.push_back(faces[i].tx[2]);
		tx1.push_back(faces[i].ty[2]);

#pragma warning(pop)

		meshTexCoords.push_back(tx1);
	}

	// Create an empty object
	happly::PLYData plyOut;

	//addVertexPositions2(plyOut, pts);
	// Add mesh data (elements are created automatically)
	plyOut.addVertexPositions(meshVertexPositions);
	//plyOut.addVertexColors(meshVertexColors);
	//plyOut.addFaceIndices(meshFaceIndices);

	addFaceIndices(plyOut, meshFaceIndices, meshTexCoords);


	// Write the object to file
	std::string plyFilename = StdUtility::replaceExtension(filename, "ply");

	std::string name = StdUtility::getName(filename);
	name += ".png";

	std::string comm = "TextureFile ";
	comm += name;

	plyOut.comments.push_back(comm);

	//plyOut.write(plyFilename, happly::DataFormat::ASCII);
	plyOut.write(plyFilename, happly::DataFormat::Binary);

	return true;
}

bool MeshT::writeToPLY(const std::string &plyFilename)
{
	MyOpenMesh mesh;
	fillToOpenMesh(mesh);

	std::vector<PointMesh> pts;
	std::vector<FaceIt> faces;

	// read open mesh...
	for (MyOpenMesh::VertexIter vit = mesh.vertices_begin(); vit != mesh.vertices_end(); ++vit)
	{
		OpenMesh::Vec3f vert = mesh.point(*vit);

		PointMesh pt;
		pt.x = vert[0];
		pt.y = vert[1];
		pt.z = vert[2];

		pts.push_back(pt);
	}

	MyOpenMesh::FaceIter it = mesh.faces_begin();

	for (; it != mesh.faces_end(); ++it)
	{
		FaceIt newFace;

		auto vertIter = mesh.fv_ccwbegin(*it);
		int vCount = 0;
		for (; vertIter != mesh.fv_ccwend(*it); ++vertIter) {
			OpenMesh::Vec3f vert = mesh.point(*vertIter);

			auto index = vertIter->idx();

			newFace.index[vCount] = index;
			++vCount;
		}

		auto hedgeIter = mesh.fh_ccwbegin(*it);
		vCount = 0;
		for (; hedgeIter != mesh.fh_ccwend(*it); ++hedgeIter) {

			const MyOpenMesh::TexCoord2D& tex = mesh.texcoord2D(*hedgeIter);

			newFace.tx[vCount] = tex[0];
			newFace.ty[vCount] = tex[1];
			++vCount;
		}

		faces.push_back(newFace);
	}

	makeMeshPLYTextures(pts, faces, plyFilename);

	return true;
}