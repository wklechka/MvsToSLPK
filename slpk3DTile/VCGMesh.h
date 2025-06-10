#pragma once

//vg
// stuff to define the mesh
#include <vcg/complex/complex.h>

#include <wrap/io_trimesh/import_ply.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export.h>
//#include <wrap/io_trimesh/import_dae.h>
//#include <wrap/io_trimesh/xmldocumentmanaging.cpp>
#include <wrap/io_trimesh/export_ply.h>
#include <wrap/ply/plylib.cpp>
#include <vcg/math/quaternion.h>
#include <vcg/complex/algorithms/create/platonic.h>
#include <vcg/complex/algorithms/update/position.h>
#include <vcg/complex/algorithms/local_optimization.h>
#include <vcg/complex/algorithms/local_optimization/tri_edge_collapse_quadric.h>

#if 0
class vcgFace; // dummy prototype
class vcgVertex;

struct vcgUsedType : public vcg::UsedTypes< vcg::Use<vcgVertex>::AsVertexType, vcg::Use<vcgFace>::AsFaceType> {};

class vcgVertex : public vcg::Vertex< vcgUsedType,
	vcg::vertex::VFAdj,
	vcg::vertex::InfoOcf,
	vcg::vertex::Coord3f,
	vcg::vertex::Color4bOcf,
	vcg::vertex::Normal3f,
	vcg::vertex::Qualityf,
	vcg::vertex::BitFlags>
{
public:
	vcg::math::Quadric<double>& Qd() { return q; }
private:
	vcg::math::Quadric<double> q;
};

class vcgFace : public vcg::Face  < vcgUsedType,
	vcg::face::VFAdj,
	vcg::face::VertexRef,
	vcg::face::Normal3f,
	vcg::face::BitFlags,
	vcg::face::WedgeTexCoord2f> {};

class vcgMesh : public vcg::tri::TriMesh< vcg::vertex::vector_ocf<vcgVertex>, std::vector<vcgFace> > {};


typedef float Scalarm;

typedef vcg::tri::BasicVertexPair<vcgVertex> VertexPair;

class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric< vcgMesh, VertexPair, MyTriEdgeCollapse, vcg::tri::QInfoStandard<vcgVertex>  > {
public:
	typedef  vcg::tri::TriEdgeCollapseQuadric< vcgMesh, VertexPair, MyTriEdgeCollapse, vcg::tri::QInfoStandard<vcgVertex>  > TECQ;
	typedef  vcgMesh::VertexType::EdgeType EdgeType;
	inline MyTriEdgeCollapse(const VertexPair& p, int i, vcg::BaseParameterClass* pp) :TECQ(p, i, pp) {}
};
#endif

using namespace vcg;
using namespace tri;

class MyVertex;
class MyEdge;
class MyFace;

struct MyUsedTypes : public UsedTypes<Use<MyVertex>::AsVertexType, Use<MyEdge>::AsEdgeType, Use<MyFace>::AsFaceType> {};

class MyVertex : public Vertex< MyUsedTypes,
	vertex::VFAdj,
	vertex::Coord3f,
	//vertex::Normal3f, noticed no normals
	vertex::Color4b,
	vertex::Mark,
	vertex::Qualityf,
	vertex::BitFlags> {
public:
	vcg::math::Quadric<double>& Qd() { return q; }
private:
	math::Quadric<double> q;
};

class MyEdge : public Edge< MyUsedTypes> {};

typedef BasicVertexPair<MyVertex> VertexPair;

class MyFace : public Face< MyUsedTypes,
	face::VFAdj,
	face::VertexRef,
	face::BitFlags,
	vcg::face::WedgeTexCoord2f> {};

// the main mesh class
class MyMesh : public vcg::tri::TriMesh<std::vector<MyVertex>, std::vector<MyFace> > {};


class MyTriEdgeCollapse : public vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QInfoStandard<MyVertex>  > {
public:
	typedef  vcg::tri::TriEdgeCollapseQuadric< MyMesh, VertexPair, MyTriEdgeCollapse, QInfoStandard<MyVertex>  > TECQ;
	typedef  MyMesh::VertexType::EdgeType EdgeType;
	inline MyTriEdgeCollapse(const VertexPair& p, int i, BaseParameterClass* pp) :TECQ(p, i, pp) {}
};
typedef float Scalarm;
