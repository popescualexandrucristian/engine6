#pragma once
#include <vulkan/vulkan.h>
#include <context.h>
#include <swapchain.h>
#include <stdio.h>
#include <utils.h>

struct shader;
struct graphics_program;

struct frame_sync
{
	VkFence render_fence;
	VkSemaphore render_semaphore;
	VkSemaphore present_semaphore;
};

struct renderer_context
{
	VkInstance instance;
	VkAllocationCallbacks* host_allocator;
	PFN_vkDebugReportCallbackEXT debug_callback;
	VkDebugReportCallbackEXT callback_extension;
	VkPhysicalDevice physical_device;
	uint32_t graphics_family_index;
	VkDevice logical_device;
	VkSurfaceKHR surface;
	VkFormat swapchain_format;
	VkFormat depth_format;
	swapchain* swapchain;
	VmaAllocator gpu_allocator;

	std::vector<frame_sync> frame_syncs;
	size_t max_frames;
	size_t current_frame;

	VkQueue graphics_queue;

	std::vector<VkCommandPool> imediate_commands_pools;
	std::vector<VkFence> imediate_commands_fences;

	//alex(todo) : remove this garbage.
	shader* vertex_shader;
	shader* fragment_shader;
	graphics_program* program;
	std::vector<VkCommandPool> commands_pools;
	std::vector<VkDescriptorPool> descriptor_pools;
	buffer_data vertex_data;

	struct vertex
	{
		float position[3];
		float pedding;
		float color[3];
		float pedding2;
	};
};

#define VK_CHECK(x, c)											\
do																\
{																\
	VkResult err = x;											\
	if (err)													\
	{															\
		VkDebugUtilsMessengerCallbackDataEXT debug_data{};		\
		char result_storage[128] = {0};							\
		sprintf(result_storage, "vulkan error : %d\n", x);		\
		debug_data.pMessage = result_storage;					\
		c->debug_callback(VK_DEBUG_REPORT_ERROR_BIT_EXT,		\
			VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,			\
				0, 0, x, nullptr, result_storage, nullptr);		\
	}															\
} while (0)