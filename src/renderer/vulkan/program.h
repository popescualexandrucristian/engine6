#pragma once
struct renderer_context;

#include <vulkan/vulkan.h>
#include <initializer_list>
#include <vector>


struct shader;
typedef std::initializer_list<const shader*> shaders;
shader* shader_init(renderer_context* context, const uint32_t* const data, size_t data_size);
void shader_destroy(renderer_context* context, shader* shader);


struct graphics_program;
struct input_attribute_data
{
	uint32_t				binding;
	std::vector<uint32_t>	offsets;
	std::vector<uint32_t>	locations;
	uint32_t				stride;
	VkVertexInputRate		input_rate;
};
typedef std::initializer_list<input_attribute_data> input_attributes;
graphics_program* graphics_program_init(renderer_context* context, shaders shaders, input_attributes vertex_input_attributes, size_t push_constant_size, bool use_depth, bool write_to_depth);
void graphics_program_destroy(renderer_context* context, graphics_program* program);