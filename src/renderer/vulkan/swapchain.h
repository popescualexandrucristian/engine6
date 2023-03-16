#pragma once
#include <vulkan/vulkan.h>

struct swapchain;
struct renderer_context;

swapchain* swapchain_init(renderer_context*);

void swapchian_destroy(swapchain*, renderer_context*);

void swapchian_update(swapchain*, renderer_context*);
