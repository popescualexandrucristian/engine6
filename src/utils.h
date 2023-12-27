#pragma once

#include <inttypes.h>
#include <tiny_gltf.h>

bool load_binary_data(const char* filePath, uint32_t** out, size_t& out_size);

bool load_binary_data(const char* filePath, char** out, size_t& out_size);