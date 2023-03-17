#pragma once
#include <vulkan/vulkan.h>
#include <context.h>
#include <swapchain.h>
#include <stdio.h>

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
	swapchain* swapchain;
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