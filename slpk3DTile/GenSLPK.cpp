#include "pch.h"
#include "GenSLPK.h"
#include "UtilityFunc.h"
//#include "Utilities/slpk3DTile/FolderNames.h"

#include "i3s/i3s_writer.h"
#include "utils/utl_png.h"
#include "utils\utl_obb.h"
#include "i3s\i3s_common.h"

#include "StdUtil\StdUtility.h"

#include <random>

// Coordinate systems used by OneButton
// #include "gdal.h"
// #include "ogr_core.h"
// #include "ogr_spatialref.h"

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <OpenMesh/Tools/Utils/Timer.hh>
#include <OpenMesh/Tools/Utils/getopt.h>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>
#include <OpenMesh/Tools/Decimater/ModIndependentSetsT.hh>

#include "CoordSys.h"

typedef std::vector<i3slib::i3s::Simple_raw_mesh> RawMeshList;

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

static bool readOpenMesh(MyOpenMesh& mesh, const std::string& inFile)
{
	OpenMesh::IO::Options ropt;
	ropt += OpenMesh::IO::Options::FaceTexCoord;
	ropt += OpenMesh::IO::Options::VertexNormal;

	std::string ifname = inFile;
	StdUtility::replaceAll(ifname, "\\", "/");

	bool rc = OpenMesh::IO::read_mesh(mesh, ifname, ropt);

	return rc;
}

static void openMeshToRawMesh_NoIndices(MyOpenMesh& mesh, i3slib::i3s::Simple_raw_mesh& rawMesh)
{
	rawMesh.vertex_count = mesh.n_faces() * 3; // times 3 for number of verts
	rawMesh.abs_xyz = new i3slib::utl::Vec3d[rawMesh.vertex_count];
	rawMesh.uv = new i3slib::utl::Vec2f[rawMesh.vertex_count];

	i3slib::utl::Vec3d* vertPtr = (i3slib::utl::Vec3d*)rawMesh.abs_xyz;
	i3slib::utl::Vec2f* uvPtr = (i3slib::utl::Vec2f*)rawMesh.uv;

	for (MyOpenMesh::FaceIter it = mesh.faces_begin(); it != mesh.faces_end(); ++it)
	{
		auto vertIter = mesh.fv_ccwbegin(*it);
		for (; vertIter != mesh.fv_ccwend(*it); ++vertIter) {
			OpenMesh::Vec3f vert = mesh.point(*vertIter);
			//const MyOpenMesh::TexCoord2D& tex = mesh.texcoord2D(*vertIter);

			(*vertPtr)[0] = vert[0];
			(*vertPtr)[1] = vert[1];
			(*vertPtr)[2] = vert[2];
			++vertPtr;
		}

		for (auto hedgeIter = mesh.fh_ccwbegin(*it); hedgeIter != mesh.fh_ccwend(*it); ++hedgeIter) {
			const MyOpenMesh::TexCoord2D& tex = mesh.texcoord2D(*hedgeIter);

			(*uvPtr)[0] = tex[0];
			(*uvPtr)[1] = 1.0 - tex[1];

			++uvPtr;
		}
	}
}

static bool addTextureToRawMesh(i3slib::i3s::Simple_raw_mesh& rawMesh, uint8_t* src_data, int width, int height)
{
	// Encode to PNG.
	std::vector<uint8_t> png_bytes;
	if (!i3slib::utl::encode_png(
		reinterpret_cast<uint8_t*>(src_data), width, height, false, png_bytes))
		return false;

	rawMesh.img.meta.alpha_status = i3slib::i3s::Texture_meta::Alpha_status::Opaque;
	rawMesh.img.meta.wrap_mode = i3slib::i3s::Texture_meta::Wrap_mode::None;
	rawMesh.img.meta.is_atlas = false;
	rawMesh.img.meta.mip0_width = width;
	rawMesh.img.meta.mip0_height = height;
	rawMesh.img.meta.mip_count = 1; //no mip
	rawMesh.img.meta.format = i3slib::i3s::Image_format::Png;

	rawMesh.img.data = i3slib::utl::Buffer::create_deep_copy<char>(
		reinterpret_cast<const char*>(png_bytes.data()),
		static_cast<int>(png_bytes.size()));

	return true;
}

template<typename T>
size_t remove_alpha_channel(T* data, size_t size)
{
	I3S_ASSERT(size % 4 == 0);
	size_t out = 3;
	for (size_t in = 4; in < size; in += 4, out += 3)
	{
		data[out] = data[in];
		data[out + 1] = data[in + 1];
		data[out + 2] = data[in + 2];
	}

	I3S_ASSERT(out * 4 == size * 3);
	return out;
}

static bool readPNG(const std::string& textureFile, std::vector<uint8_t>& data, int& width, int& height)
{
	std::vector<char>& dataRef = *((std::vector<char>*) & data); // had to cast it
	if (!i3slib::utl::read_png_from_file(textureFile, &width, &height, &dataRef))
	{
		std::cout << "Failed to load color bitmap from file." << std::endl;
		return false;
	}

	//int size = width;

	// The current png reader implementation always produces RGBA output for color images.
	// We have to strip alpha channel here.
	I3S_ASSERT(data.size() == width * height * 4);
	data.resize(remove_alpha_channel(data.data(), data.size()));
	I3S_ASSERT(data.size() == width * height * 3);

	return true;
}

static auto fillRawMesh(const std::string& meshFilename, MyOpenMesh& mesh, i3slib::i3s::Simple_raw_mesh& rMesh)
{
	openMeshToRawMesh_NoIndices(mesh, rMesh);

	std::string pngFile;
	std::vector<uint8_t> texture;
	int width, height;

	pngFile = StdUtility::replaceExtension(meshFilename, "png");
	//getSplitPNG(meshFilename, pngFile);

	if (!readPNG(pngFile, texture, width, height)) {
		return false;
	}

	// add texture
	addTextureToRawMesh(rMesh, texture.data(), width, height);

	return true;
}

static void addOffset(i3slib::i3s::Simple_raw_mesh& mesh, i3slib::utl::Vec3d& offset)
{
	i3slib::utl::Vec3d* ptr = (i3slib::utl::Vec3d*)mesh.abs_xyz;

	for (int i = 0; i < mesh.vertex_count; ++i) {
		i3slib::utl::Vec3d& cog = ptr[i];
		cog += offset;
	}
}


static void toLatLonMesh(const std::string& projFile , i3slib::i3s::Simple_raw_mesh& mesh)
{
	// don't check - may be using Summit project/Blue Marble for coord conversion
// 	if (!StdUtility::fileExists(projFile)) {
// 		// unable to transform without the projection file
// 		return;
// 	}

	CoordConverter cc;
	cc.setFromFile(projFile);
	
	i3slib::utl::Vec3d* ptr = (i3slib::utl::Vec3d*)mesh.abs_xyz;
	for (int i = 0; i < mesh.vertex_count; ++i) {
		i3slib::utl::Vec3d& cog = ptr[i];
		cc.transform(cog.x, cog.y, cog.z);
	}
}

static i3slib::i3s::Layer_writer::Var create_writer(const std::filesystem::path& slpk_path)
{
	i3slib::i3s::Ctx_properties ctx_props(i3slib::i3s::Max_major_versions({}));
	//i3slib::i3s::set_geom_compression(ctx_props.geom_encoding_support, i3slib::i3s::Geometry_compression::Draco, true);
	//i3slib::i3s::set_gpu_compression(ctx_props.gpu_tex_encoding_support, i3slib::i3s::GPU_texture_compression::ETC_2, true);
	auto writer_context = i3slib::i3s::create_i3s_writer_context(ctx_props);

	i3slib::i3s::Layer_meta meta;
	meta.type = i3slib::i3s::Layer_type::Mesh_IM;
	meta.name = i3slib::utl::to_string(slpk_path.stem().generic_u8string());
	meta.desc = "Generated with Datem";
	meta.sr.wkid = 4326;
	meta.uid = meta.name;
	meta.normal_reference_frame = i3slib::i3s::Normal_reference_frame::Not_set;

	// Temp hack to make the output SLPKs be exactly the same in different runs on the same input.
	// TODO: add command line parameter for the timestamp?
	meta.timestamp = 1;

	std::unique_ptr<i3slib::i3s::Layer_writer> writer(
		i3slib::i3s::create_mesh_layer_builder(writer_context, slpk_path));

	if (writer)
		writer->set_layer_meta(meta);
	return writer;
}

static double screen_size_to_area(double pixels)
{
	constexpr double c_pi_over_4 = M_PI * 0.25;
	return pixels * pixels * c_pi_over_4;
}

#if 0
bool write_i3s(std::vector<RawMeshList>& rawMeshes, const std::string& slpkFilename)
{
	// add nodes i3s writer...
	std::filesystem::path slpk_file_path(slpkFilename);
	auto writer = create_writer(slpk_file_path);
	if (!writer) {
		std::cout << "creating writer failed: " << slpkFilename.c_str() << std::endl;
		return false;
	}


	i3slib::i3s::Node_id node_id = 0;

	// the higher up the less detail

	int depth = int(rawMeshes.size());

	std::vector<i3slib::i3s::Node_id> curParentNodes;
	for (int i = 0; i < rawMeshes.size(); ++i) {
		auto& rMeshList = rawMeshes[i];

		std::vector<i3slib::i3s::Node_id> NEWparentNodes;
		for (int j = 0; j < rMeshList.size(); ++j) {

			auto& rMesh = rMeshList[j];

			// bug in i3s library if indices are used...
			// need to fixe..
	// 		rMesh.index_count = 0;
	// 		rMesh.indices = nullptr;
	// 		rMesh.vertex_count = 300;

			i3slib::i3s::Simple_node_data node_data;
			node_data.node_depth = depth;

			// 			if (depth != rawMeshes.size()) {
			// 				node_data.children.push_back(node_id - 1);
			// 			}
			if (curParentNodes.size() > 0) {
				node_data.children.push_back(curParentNodes[j]);
			}

			node_data.lod_threshold = screen_size_to_area(500);

			bool status = writer->create_mesh_from_raw(rMesh, node_data.mesh) == IDS_I3S_OK;


			if (!status) {
				std::cout << "create_mesh_from_raw failed: " << std::endl;
				return false;
			}

			status = writer->create_node(node_data, node_id) == IDS_I3S_OK;
			if (!status) {
				std::cout << "create_node failed: " << std::endl;
				return false;
			}

			NEWparentNodes.push_back(node_id);

			++node_id;
		}
		curParentNodes = NEWparentNodes;

		--depth;
	}


	// Add a root node on top of everything.
	i3slib::i3s::Simple_node_data node_data;
	node_data.node_depth = 0;

	//node_data.children.push_back(node_id - 1);
	std::copy(curParentNodes.begin(), curParentNodes.end(), std::back_inserter(node_data.children));

	i3slib::i3s::status_t stat = writer->create_node(node_data, node_id);
	if (stat != IDS_I3S_OK)
		return false;

	printf("SLPK writing started \n");

	if (writer->save() != IDS_I3S_OK)
		return false;

	return true;
}
#endif

void deleteSimple_rawMesh(i3slib::i3s::Simple_raw_mesh& mesh)
{
	delete[] mesh.abs_xyz;
	delete[] mesh.uv;

	delete[] mesh.indices;

	delete[] mesh.fid_values;
	delete[] mesh.fids_indices;

	mesh.index_count = 0;
	mesh.vertex_count = 0;
	mesh.fid_value_count = 0;
	mesh.abs_xyz = nullptr;
	mesh.uv = nullptr;
	mesh.indices = nullptr;
	mesh.fid_values = nullptr;
	mesh.fids_indices = nullptr;
}

#if 0
static bool writeAsSLPK_gridObjs(SlpkOptions& opt)
{
	//lodSplitfiles has grided levels and already decimated
	if (opt.lodSplitfiles.size() == 0)
		return false;

	double boxData[9];
	bool hadBox = slpkUtilityFunc::boxFile(opt.boxFilename, boxData);

	i3slib::utl::Vec3d offset(boxData[0], boxData[1], boxData[2]);

	struct ThreadData
	{
		std::string filename;
		i3slib::i3s::Simple_raw_mesh* rMesh;
	};
	auto processToRaw = [](std::vector<ThreadData>& td)
		{
			for (auto& data : td) {
				std::string ifname = data.filename;
				StdUtility::replaceAll(ifname, "\\", "/");

				MyOpenMesh mesh;
				if (!readOpenMesh(mesh, ifname))
					return false;

				fillRawMesh(ifname, mesh, *data.rMesh);
			}
			return true;
		};


	std::vector<std::thread> threads;
	std::vector<RawMeshList> rawMeshes;
	rawMeshes.resize(opt.lodSplitfiles.size());

	int lev = 0;
	for (auto& lodLevelFiles : opt.lodSplitfiles) {
		rawMeshes[lev].resize(lodLevelFiles.size());
		++lev;
	}

	std::vector<ThreadData> processList;
	lev = 0;
	for (auto& lodLevelFiles : opt.lodSplitfiles) {
		int fileNum = 0;
		for (auto& filename : lodLevelFiles) {
			ThreadData data;
			data.filename = filename;
			data.rMesh = &rawMeshes[lev][fileNum];

			processList.push_back(data);

			++fileNum;
		}
		++lev;
	}

	//int numThreads = 4;
	int numThreads = 1;
	if (numThreads > processList.size()) {
		numThreads = processList.size();
	}

	std::random_device rd;
	std::mt19937 gMt(rd());
	//std::shuffle(processList.begin(), processList.end(), gMt);

	auto itemsToProcess = StdUtility::split(processList, numThreads);

	// each thread is given a list of meshes to run against
	for (auto& listsToRun : itemsToProcess) {
		threads.push_back(std::thread(processToRaw, std::ref(listsToRun)));
	}
	for (auto& aThread : threads) {
		aThread.join();
	}


	// add offset and 
	// convert to LAT/LON
	for (auto& rMeshList : rawMeshes) {
		for (auto& rMesh : rMeshList) {
			addOffset(rMesh, offset);
			toLatLonMesh(opt.prjFilename, rMesh);
		}
	}

	bool status = write_i3s(rawMeshes, opt.outSlpkFilename);

	for (auto& rMeshList : rawMeshes) {
		for (auto& rMesh : rMeshList) {
			deleteSimple_rawMesh(rMesh);
		}
	}

	return true;
}
#endif

static bool writeAsSLPK_gridObjs2(SlpkOptions& opt)
{
	//lodSplitfiles has grided levels and already decimated
	if (opt.lodSplitfiles.size() == 0)
		return false;

	double boxData[9];
	bool hadBox = slpkUtilityFunc::boxFile(opt.boxFilename, boxData);
	i3slib::utl::Vec3d offset(boxData[0], boxData[1], boxData[2]);


	// add nodes i3s writer...
	std::filesystem::path slpk_file_path(opt.outSlpkFilename);
	auto writer = create_writer(slpk_file_path);
	if (!writer) {
		std::cout << "creating writer failed: " << opt.outSlpkFilename.c_str() << std::endl;
		return false;
	}

	i3slib::i3s::Node_id node_id = 0;
	int depth = int(opt.lodSplitfiles.size());

	std::vector<i3slib::i3s::Node_id> curParentNodes;

	for (auto& lodLevelFiles : opt.lodSplitfiles) {

		std::vector<i3slib::i3s::Node_id> NEWparentNodes;

		int meshIndex = 0;
		for (auto& meshObjFile : lodLevelFiles) {
			std::string ifname = meshObjFile;
			StdUtility::replaceAll(ifname, "\\", "/");

			std::cout << "SLPK processing: " << StdUtility::getName(ifname).c_str() << std::endl;

			MyOpenMesh mesh;
			if (!readOpenMesh(mesh, ifname))
				return false;

			i3slib::i3s::Simple_raw_mesh rMesh;
			fillRawMesh(ifname, mesh, rMesh);

			addOffset(rMesh, offset);
			toLatLonMesh(opt.prjFilename, rMesh);

			// write it out..?
			i3slib::i3s::Simple_node_data node_data;
			node_data.node_depth = depth;
			if (curParentNodes.size() > 0) {
				node_data.children.push_back(curParentNodes[meshIndex]);
			}

			node_data.lod_threshold = screen_size_to_area(500);
			bool status = writer->create_mesh_from_raw(rMesh, node_data.mesh) == IDS_I3S_OK;
			if (!status) {
				std::cout << "create_mesh_from_raw failed: " << std::endl;
				return false;
			}
			status = writer->create_node(node_data, node_id) == IDS_I3S_OK;
			if (!status) {
				std::cout << "create_node failed: " << std::endl;
				return false;
			}

			NEWparentNodes.push_back(node_id);
			++node_id;
			++meshIndex;

			deleteSimple_rawMesh(rMesh);
		}
		curParentNodes = NEWparentNodes;

		--depth;
	}

	// Add a root node on top of everything.
	i3slib::i3s::Simple_node_data node_data;
	node_data.node_depth = 0;

	//node_data.children.push_back(node_id - 1);
	std::copy(curParentNodes.begin(), curParentNodes.end(), std::back_inserter(node_data.children));

	i3slib::i3s::status_t stat = writer->create_node(node_data, node_id);
	if (stat != IDS_I3S_OK)
		return false;

	printf("SLPK writing started \n");

	if (writer->save() != IDS_I3S_OK)
		return false;

	return true;
}

int doVTK_SLPK(SlpkOptions& opt)
{
	if (opt.slpkTileLimit < 0) {
		// new more mem efficient
		writeAsSLPK_gridObjs2(opt);
	}
	else {
		// see if we should make more than 1 slpk
		bool generateMultipleSLPK = false;
		if (opt.lodSplitfiles.size() > 0) {
			if (opt.lodSplitfiles[0].size() > opt.slpkTileLimit) {
				generateMultipleSLPK = true;
			}
		}
		if (generateMultipleSLPK) {
			// generate multiple
			int numToGen = (opt.lodSplitfiles[0].size() / opt.slpkTileLimit) + 1;
			for (int i = 0; i < numToGen; ++i) {
				SlpkOptions newOpt = opt;
				
				// make a new filename
				std::string addOn = StdUtility::string_format("_%d", i + 1);
				newOpt.outSlpkFilename = StdUtility::appendName(opt.outSlpkFilename, addOn);
			
				// make a new list...
				newOpt.lodSplitfiles.clear();
				for (auto lodFiles : opt.lodSplitfiles) {
					auto splitLists = StdUtility::split(lodFiles, numToGen);
					newOpt.lodSplitfiles.push_back(splitLists[i]);
				}

				writeAsSLPK_gridObjs2(newOpt);
			}
		}
		else {
			writeAsSLPK_gridObjs2(opt);
		}
	}

	return 0;
}