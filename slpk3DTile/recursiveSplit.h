#pragma once
#include "SplitMesh.h"


int recursiveSplitXY(MeshT& mesh, int depth, Box3 bounds, std::vector<MeshT>& meshes);
bool splitUpMesh(const std::string& ifname, const std::string& outFolder, int splitDiv, bool forceSquare, int lod);

