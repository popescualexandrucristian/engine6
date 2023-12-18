#pragma once

#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_program_vulkan.h>

constexpr uint32_t initial_width = 800;
constexpr uint32_t initial_height = 600;
constexpr bool use_vsync = true;
constexpr bool use_depth = true;

struct user_data
{
	acp_vulkan::shader* vertex_shader;
	acp_vulkan::shader* fragment_shader;
	acp_vulkan::graphics_program* program;
	std::vector<VkCommandPool> commands_pools;
	std::vector<VkDescriptorPool> descriptor_pools;
	acp_vulkan::buffer_data vertex_data;

	struct vertex
	{
		float position[3];
		float pedding;
		float color[3];
		float pedding2;
	};
};

acp_vulkan::renderer_context* init_user_render_context();