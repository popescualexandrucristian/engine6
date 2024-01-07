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
		free_binary_data(local);
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

void free_binary_data(void* data)
{
	_aligned_free(data);
}
