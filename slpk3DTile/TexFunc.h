#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <assert.h>

void build_downsampled_textures(int size, const uint8_t* data, int min_size, std::vector<std::vector<uint8_t>>& textures, std::vector<int>& texturesSizes);
bool writePNG(const std::string& pngFile, int w, int h, const char* buffer, int n_bytes);
bool readPNG(const std::string& textureFile, std::vector<uint8_t>& data, int& width, int& height);

template<typename T>
size_t remove_alpha_channel(T* data, size_t size)
{
	assert(size % 4 == 0);
	size_t out = 3;
	for (size_t in = 4; in < size; in += 4, out += 3)
	{
		data[out] = data[in];
		data[out + 1] = data[in + 1];
		data[out + 2] = data[in + 2];
	}

	assert(out * 4 == size * 3);
	return out;
}