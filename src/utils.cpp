#include <utils.h>
#include <stdio.h>
#include <malloc.h>

static bool load_binary_data_internal(const char* filePath, size_t allignment_for_out, char** out, size_t& out_size)
{
	FILE* file = fopen(filePath, "rb");
	fseek(file, 0, SEEK_END);
	long total_bytes_in_file = ftell(file);
	size_t total_bytes = ((total_bytes_in_file + allignment_for_out - 1) / allignment_for_out) * allignment_for_out;
	fseek(file, 0, SEEK_SET);
	char* local = reinterpret_cast<char*>(_aligned_malloc(total_bytes, allignment_for_out));

	size_t total_bytes_to_read = total_bytes_in_file;
	size_t total_read_bytes = 0;
	while (size_t read_bytes = fread(local, 1, total_bytes_to_read, file))
	{
		if (read_bytes == 0)
			break;

		total_read_bytes += read_bytes;

		if (read_bytes >= total_bytes_to_read)
			break;

		total_bytes_to_read -= read_bytes;
	}

	if (total_bytes_in_file != total_read_bytes)
	{
		_aligned_free(local);
		out = nullptr;
		out_size = 0;
		return false;
	}
	*out = local;
	out_size = total_bytes / allignment_for_out;
	fclose(file);
	return true;
}

bool load_binary_data(const char* filePath, uint32_t** out, size_t& out_size)
{
	return load_binary_data_internal(filePath, alignof(uint32_t), reinterpret_cast<char**>(out), out_size);
}

bool load_binary_data(const char* filePath, char** out, size_t& out_size)
{
	return load_binary_data_internal(filePath, alignof(char), reinterpret_cast<char**>(out), out_size);
}

void read_gltf_data_to_array(const tinygltf::Model* model, size_t accessor, size_t element_size, char* target_array, size_t target_array_stride)
{
	const tinygltf::BufferView& buffer_view = model->bufferViews[model->accessors[accessor].bufferView];
	const tinygltf::Buffer& buffer = model->buffers[buffer_view.buffer];
	const unsigned char* source_data = buffer.data.data() + buffer_view.byteOffset;
	for (size_t ii = 0; ii < model->accessors[accessor].count; ++ii)
	{
		const unsigned char* source_ptr = source_data + ii * element_size;
		char* dest_ptr = target_array + (target_array_stride * ii);
		memcpy(dest_ptr, source_ptr, element_size);
	}
}
