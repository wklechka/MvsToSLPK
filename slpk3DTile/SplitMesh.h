#pragma once
#include "MyOpenMesh.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <string>
//#include "EigenDecimator.h"
#include <math.h>
#include "StdUtil\StdUtility.h"
#include "SplitMeshHelper.h"

#define MapDict std::unordered_map 
#define LIST_Type std::vector
//#define HashSet std::set
#define HashSet std::unordered_set

class MeshT
{
public:
	MeshT() {}

public:
	MeshT(LIST_Type<Vertex3> &vertices, LIST_Type<Vertex2> &textureVertices,
		LIST_Type<FaceT> &faces, LIST_Type<Material> &materials)
	{
		set(vertices, textureVertices, faces, materials);
	}


	int numFaces() { return _faces.size(); }

	void set(LIST_Type<Vertex3> &vertices, LIST_Type<Vertex2> &textureVertices,
		LIST_Type<FaceT> &faces, LIST_Type<Material> &materials)
	{
		_vertices = vertices;
		_textureVertices = textureVertices;
		_faces = faces;
		_materials = materials;
	}

	void setFrom(MyOpenMesh& mesh, const std::string& ifname);
	void fillToOpenMesh(MyOpenMesh& mesh);
	bool writeToObj(const std::string objFilename);
	bool writeToPLY(const std::string& plyFilename);

	template <typename T>
	int addIndex(MapDict<T, int> &dictionary, T item)
	{
		if (dictionary.find(item) == dictionary.end())
		{
			dictionary[item] = dictionary.size();
		}
		return dictionary[item];
	}

	template <typename T>
	LIST_Type<T> orderByVal(MapDict<T, int>& mapDict)
	{
		LIST_Type<T> orderedList;
		orderedList.resize(mapDict.size());

		for (auto iter = mapDict.begin(); iter != mapDict.end(); ++iter) {
			orderedList[iter->second] = iter->first;
		}
		return orderedList;
	}

	int splitMesh(IVertexUtilsBase* utils, double q, MeshT& left, MeshT& right);

	std::string meshFile;

	void regenerateTextures(const std::string& targetFolder, const std::string& objFilename);
	void regenerateTexturesCopy(const std::string& targetFolder, const std::string& objFilename);

private:

	void addCutTriLeft2D(IVertexUtilsBase* utilsBase, double q, int indexVL,
		int indexVR1, int indexVR2,
		MapDict<Vertex3, int>& leftVertices, MapDict<Vertex3, int>& rightVertices,
		int indexTextureVL, int indexTextureVR1, int indexTextureVR2,
		MapDict<Vertex2, int>& leftTextureVertices, MapDict<Vertex2, int>& rightTextureVertices,
		int materialIndex, LIST_Type<FaceT>& leftFaces, LIST_Type<FaceT>& rightFaces);

	
	void addCutTriRight2D(IVertexUtilsBase* utilsBase, double q, int indexVR,
		int indexVL1, int indexVL2,
		MapDict<Vertex3, int>& leftVertices, MapDict<Vertex3, int>& rightVertices,
		int indexTextureVR, int indexTextureVL1, int indexTextureVL2,
		MapDict<Vertex2, int>& leftTextureVertices, MapDict<Vertex2, int>& rightTextureVertices,
		int materialIndex, LIST_Type<FaceT>& leftFaces, LIST_Type<FaceT>& rightFaces);

public:
	Vertex3 baricenter();


private:
	void removeUnused();
	

	const double MinValue = -1.7976931348623157E+308;
	const double MaxValue = 1.7976931348623157E+308;

public:

	Box3 calcBounds();
	
	class Edge
	{
	public:
		int V1Index;
		int V2Index;

		Edge(int v1Index, int v2Index)
		{
			if (v1Index > v2Index)
			{
				V1Index = v2Index;
				V2Index = v1Index;
			}
			else
			{
				V1Index = v1Index;
				V2Index = v2Index;
			}
		}

		bool operator ==(const Edge& b) const
		{
			return b.V1Index == V1Index && b.V2Index == V2Index;
		}

		bool operator !=(const Edge& b) const
		{
			return !operator ==(b);
		}
	};

	class Rectangle
	{
	public:
		double X, Y, Width, Height;

		Rectangle(double minX, double minY, double w, double h)
		{
			X = minX;
			Y = minY;
			Width = w;
			Height = h;
		}
	};

protected:
	Rectangle getClusterRect(LIST_Type<int>& cluster);

	void calculateMaxMinAreaRect(std::vector<Rectangle>& clustersRects, int textureWidth, int textureHeight,
		double& maxWidth, double& maxHeight, double& textureArea);

	bool binPackTextures(const std::string& targetFolder, int materialIndex, LIST_Type<LIST_Type<int>>& clusters, const std::string& objFilename);

	MapDict<int, LIST_Type<int>> getFacesMapper(MapDict<MeshT::Edge, LIST_Type<int>>& edgesMapper);
	MapDict<MeshT::Edge, LIST_Type<int>> getEdgesMapper(LIST_Type<int>& facesIndexes);
	std::vector<std::vector<int>> getFacesByMaterial();

protected:
	LIST_Type<Vertex3> _vertices;
	LIST_Type<Vertex2> _textureVertices;
	LIST_Type<FaceT> _faces;
	LIST_Type<Material> _materials;

	Box3 _bounds;
};

template<>
struct std::hash<MeshT::Edge>
{
	std::size_t operator()(const MeshT::Edge& s) const noexcept
	{
		std::size_t h1 = std::hash<int>{}(s.V1Index);
		std::size_t h2 = std::hash<int>{}(s.V2Index);
		return h1 ^ (h2 << 1); // or use boost::hash_combine
	}
};
