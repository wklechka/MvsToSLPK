#include "recursiveSplit.h"
#include "StdUtil\StdUtility.h"
#include "SplitName.h"

static VertexUtilsX Xutils;
static VertexUtilsY Yutils;

// cut up a mesh into a vector of meshes
int recursiveSplitXY(MeshT &mesh, int depth, Box3 bounds, std::vector<MeshT>& meshes)
{
	if (depth == 0)
	{
		if (mesh.numFaces() > 0)
			meshes.push_back(mesh);
		return 0;
	}

	auto center = bounds.center();
	MeshT left, right;
	auto count = mesh.splitMesh(&Xutils, center.X, left, right);

	MeshT topLeft, topRight, bottomLeft, bottomRight;
	count += left.splitMesh(&Yutils, center.Y, topLeft, topRight);
	count += right.splitMesh(&Yutils, center.Y, bottomLeft, bottomRight);

	Box3 xbounds[2];
	Box3 ybounds1[2];
	Box3 ybounds2[2];

	bounds.splitBox(IVertexUtilsBase::X, xbounds);
	xbounds[0].splitBox(IVertexUtilsBase::Y, ybounds1);
	xbounds[1].splitBox(IVertexUtilsBase::Y, ybounds2);

	auto nextDepth = depth - 1;

	if (topLeft.numFaces() > 0) {
		count += recursiveSplitXY(topLeft, nextDepth, ybounds1[0], meshes);
	}

	if (bottomLeft.numFaces() > 0) {
		count += recursiveSplitXY(bottomLeft, nextDepth, ybounds2[0], meshes);
	}

	if (topRight.numFaces() > 0) {
		count += recursiveSplitXY(topRight, nextDepth, ybounds1[1], meshes);
	}

	if (bottomRight.numFaces() > 0) {
		count += recursiveSplitXY(bottomRight, nextDepth, ybounds2[1], meshes);
	}

	return count;
}

bool readOpenMesh(MyOpenMesh& mesh, const std::string& inFile)
{
	OpenMesh::IO::Options ropt;
	ropt += OpenMesh::IO::Options::FaceTexCoord;
	ropt += OpenMesh::IO::Options::VertexNormal;

	std::string ifname = inFile;
	StdUtility::replaceAll(ifname, "\\", "/");

	bool rc = OpenMesh::IO::read_mesh(mesh, ifname, ropt);

	if (!rc) {
		return false;
	}

	// 	if (rc)
	// 		std::cout << "  read in " << std::endl;
	// 	else
	// 	{
	// 		std::cout << "  read failed\n" << std::endl;
	// 		return false;
	// 	}

	return true;
}

// splitDiv is how many level to split. 1 - 4 boxes, 2- 16 boxes, 3 - 64 boxes 
bool splitUpMesh(const std::string& ifname, const std::string& outFolder, int splitDiv, bool forceSquare, int lod)
{
	MyOpenMesh mesh;
	if (!readOpenMesh(mesh, ifname)) {
		return false;
	}

	MeshT splitMesh;
	splitMesh.setFrom(mesh, ifname);

	std::vector<MeshT> meshes;

	Box3 meshBounds = splitMesh.calcBounds();
	if (forceSquare) {
		if (meshBounds.width() > meshBounds.height()) {
			// increase height to match width
			meshBounds.Max.Y = meshBounds.Min.Y + meshBounds.width();
		}
		else if (meshBounds.width() < meshBounds.height()) {
			meshBounds.Max.X = meshBounds.Min.X + meshBounds.height();
		}
	}
	recursiveSplitXY(splitMesh, splitDiv, meshBounds, meshes);

	// special case of not splitting
	if (splitDiv == 0 && meshes.size() == 1) {
		int meshCount = 0;
		auto& mesh = meshes[0];
		mesh.meshFile = outFolder;
		mesh.meshFile += StdUtility::string_format("\\" SplitName_ "%d.ply", ++meshCount);

		mesh.writeToPLY(mesh.meshFile);

		std::string textureFile = StdUtility::replaceExtension(mesh.meshFile, "png");
		std::string orgTextureFile = StdUtility::replaceExtension(ifname, "png");

		StdUtility::copyFile(orgTextureFile, textureFile);

		return true;
	}

	// non threaded version
#if 0
	int meshCount = 0;
	for (auto& mesh : meshes) {
		std::string meshFile = outFolder;
		meshFile += StdUtility::string_format("\\" SplitName_ "%d.obj", meshCount);

		mesh.regenerateTextures(outFolder, meshFile);
		mesh.writeToObj(meshFile);
		++meshCount;
	}

#else
	// thread IT!
	std::vector<std::thread> threads;

	auto threadToRun = [lod](std::vector<MeshT*> meshList, std::string outFolder)
		{
			for (auto& mesh : meshList) {
				if (lod > 0) {
					// regenerateTextures not working properly on lower LODs
					mesh->regenerateTexturesCopy(outFolder, mesh->meshFile);
				}
				else {
					// this works absolutely great at LOD 0
					// the decimated meshes freak out
					mesh->regenerateTextures(outFolder, mesh->meshFile);
				}
				
				mesh->writeToPLY(mesh->meshFile);
			}
		};

	int meshCount = 0;
	for (auto& mesh : meshes) {
		mesh.meshFile = outFolder;
		mesh.meshFile += StdUtility::string_format( "\\" SplitName_ "%d.ply", ++meshCount);
	}

	std::vector<MeshT*> ptrToMeshes;
	for (int meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
		ptrToMeshes.push_back(&meshes[meshIndex]);
	}

	int numThreads = 4;
	if (numThreads > meshes.size()) {
		numThreads = meshes.size();
	}
	std::vector< std::vector<MeshT*>> ptrsToThread = StdUtility::split(ptrToMeshes, numThreads);
	
	// each thread is given a list of meshes to run against
	for (auto& listsToRun : ptrsToThread) {
		threads.push_back(std::thread(threadToRun, listsToRun, outFolder));
	}
	// wait for each thread to end
	for (auto& aThread : threads) {
		aThread.join();
	}
#endif

	return true;
}