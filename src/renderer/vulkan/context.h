#pragma once
#include <vulkan/vulkan.h>
#include <functional>

struct renderer_context;

renderer_context* renderer_init(uint32_t width, uint32_t height, bool use_vsync, bool use_depth);

bool renderer_resize(renderer_context* context, uint32_t width, uint32_t height, bool use_vsync, bool use_depth);

void renderer_update(renderer_context*, double);

void immediate_submit(renderer_context* context, std::function<void(VkCommandBuffer cmd)>&& function);

void renderer_shutdown(renderer_context*);

//os interface

PFN_vkDebugReportCallbackEXT get_renderer_debug_callback();

const char** get_renderer_debug_layers();
size_t get_renderer_debug_layers_count();

const char** get_renderer_extensions();
size_t get_renderer_extensions_count();

bool get_renderer_device_supports_presentation(VkPhysicalDevice physical_device, uint32_t family_index);

VkSurfaceKHR create_renderer_surface(VkInstance instance);
void destroy_renderer_surface(VkSurfaceKHR surface, VkInstance instance);