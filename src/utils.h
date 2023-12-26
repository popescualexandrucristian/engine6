#pragma once

#include <inttypes.h>
#include <tiny_gltf.h>

bool load_binary_data(const char* filePath, uint32_t** out, size_t& out_size);

bool load_binary_data(const char* filePath, char** out, size_t& out_size);

void read_gltf_data_to_array(const tinygltf::Model* model, size_t accessor, size_t element_size, char* target_array, size_t target_array_stride);