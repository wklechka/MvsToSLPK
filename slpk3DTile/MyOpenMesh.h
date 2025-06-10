#pragma once

#pragma warning(disable: 4267)
#pragma warning(disable: 4244)

// Open mesh for read PLY
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <OpenMesh/Tools/Utils/Timer.hh>
#include <OpenMesh/Tools/Utils/getopt.h>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>
#include <OpenMesh/Tools/Decimater/ModIndependentSetsT.hh>


struct MyMConvertTraits : public OpenMesh::DefaultTraits
{
	VertexAttributes(OpenMesh::Attributes::Normal |
		OpenMesh::Attributes::Color);
	// 	VertexAttributes(OpenMesh::Attributes::Normal |
	// 		OpenMesh::Attributes::Color |
	// 		OpenMesh::Attributes::TexCoord2D);
	HalfedgeAttributes(OpenMesh::Attributes::PrevHalfedge |
		OpenMesh::Attributes::TexCoord2D);
	FaceAttributes(OpenMesh::Attributes::Normal |
		OpenMesh::Attributes::Color |
		OpenMesh::Attributes::TexCoord2D);
};


typedef OpenMesh::TriMesh_ArrayKernelT<MyMConvertTraits> MyOpenMesh;