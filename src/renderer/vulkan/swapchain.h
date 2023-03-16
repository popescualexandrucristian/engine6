#pragma once
#include <vulkan/vulkan.h>

struct swapchain;
struct renderer_context;

swapchain* swapchain_init(renderer_context* context, uint32_t width, uint32_t height, bool use_vsync);

bool swapchian_update(swapchain* swapchain, renderer_context* context, uint32_t new_width, uint32_t new_height, bool use_vsync);

void swapchian_destroy(swapchain*, renderer_context*);