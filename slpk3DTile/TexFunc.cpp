#include "pch.h"
#include "TexFunc.h"
#include <assert.h>

#include "vtkPNGWriter.h"
#include "vtkPNGReader.h"
#include "vtkNew.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkImageExtractComponents.h"

#include "vtkProcess.h"

#include "i3s/i3s_writer.h"
#include "utils/utl_png.h"
#include "utils\utl_obb.h"
#include "i3s\i3s_common.h"

// textures are assumed square

uint8_t avg4(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4)
{
	return static_cast<uint8_t>((c1 + c2 + c3 + c4 + 2) / 4);
}

// textures are assumed square
void build_downsampled_textures(int size, const uint8_t* data, int min_size, std::vector<std::vector<uint8_t>>& textures, std::vector<int>& texturesSizes)
{
	while (size > min_size)
	{
		assert((size % 2) == 0);
		const auto s = size / 2;
		const auto stride = size * 3;

		std::vector<uint8_t> texture;
		texture.reserve(s * s * 3);

		const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
		for (int y = 0; y < s; y++)
		{
			auto p1 = p + stride;
			for (int x = 0; x < s; x++, p += 6, p1 += 6)
			{
				// Get average R,G,B values over 2 * 2 block of pixels.
				texture.push_back(avg4(p[0], p[3], p1[0], p1[3]));
				texture.push_back(avg4(p[1], p[4], p1[1], p1[4]));
				texture.push_back(avg4(p[2], p[5], p1[2], p1[5]));
			}
			p = p1;
		}

		textures.emplace_back(std::move(texture));
		texturesSizes.push_back(s);
		data = textures.back().data();
		size = s;
	}
}

#if 0
void build_downsampled_textures(int size, const uint8_t* data, int min_size, std::vector<std::vector<uint8_t>>& textures, std::vector<int>& texturesSizes)
{
	while (size > min_size)
	{
		I3S_ASSERT((size % 2) == 0);
		const auto s = size / 2;
		const auto stride = size * 3;

		std::vector<uint8_t> texture;
		texture.reserve(s * s * 3);

		const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
		for (int y = 0; y < s; y++)
		{
			auto p1 = p + stride;
			for (int x = 0; x < s; x++, p += 6, p1 += 6)
			{
				// Get average R,G,B values over 2 * 2 block of pixels.
				texture.push_back(avg4(p[0], p[3], p1[0], p1[3]));
				texture.push_back(avg4(p[1], p[4], p1[1], p1[4]));
				texture.push_back(avg4(p[2], p[5], p1[2], p1[5]));
			}
			p = p1;
		}

		textures.emplace_back(std::move(texture));
		texturesSizes.push_back(s);
		data = textures.back().data();
		size = s;
	}
}
#endif

bool writePNG(const std::string& pngFile, int w, int h, const char* buffer, int n_bytes)
{
	// assume 3 channel comming in...

	vtkNew<vtkImageData> oimage;
	oimage->SetDimensions(w, h, 1);
	oimage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);


	uint8_t* ptrScr = (uint8_t*)buffer;
	unsigned char* ipos = static_cast<unsigned char*>(oimage->GetScalarPointer());
	for (int i = 0; i < n_bytes; i += 3) {
		*(ipos++) = *(ptrScr++);
		*(ipos++) = *(ptrScr++);
		*(ipos++) = *(ptrScr++);
		*(ipos++) = 255;
	}


	vtkNew<vtkImageFlip> flip;
	flip->SetFilteredAxis(1);
	flip->SetInputData(oimage);

	vtkNew<vtkPNGWriter> pngWriter;
	pngWriter->SetInputConnection(flip->GetOutputPort());
	pngWriter->SetFileName(pngFile.c_str());
	pngWriter->Write();

	return true;
}


bool readPNG(const std::string& textureFile, std::vector<uint8_t>& data, int& width, int& height)
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