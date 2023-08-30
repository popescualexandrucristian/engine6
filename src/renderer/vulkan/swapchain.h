#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <utils.h>

struct swapchain
{
	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> views;
	std::vector<image_data> depth_images;
	std::vector<VkImageView> depth_views;
	uint32_t width, height;
	bool vsync;
};

struct renderer_context;

swapchain* swapchain_init(renderer_context* context, uint32_t width, uint32_t height, bool use_vsync, bool use_depth);

bool swapchian_update(swapchain* swapchain, renderer_context* context, uint32_t new_width, uint32_t new_height, bool use_vsync, bool use_depth);

uint32_t acquire_next_image(swapchain* swapchain, renderer_context* context, VkFence fence, VkSemaphore semaphore);

void swapchian_destroy(swapchain*, renderer_context*);