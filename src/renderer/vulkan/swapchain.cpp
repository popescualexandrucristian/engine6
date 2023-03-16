#include <swapchain.h>
#include <context_internal.h>
#include <context.h>
#include <algorithm>
#include <vector>

struct swapchain
{
	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;
	uint32_t width, height;
	bool vsync;
};

static VkPresentModeKHR get_present_mode(renderer_context* context, bool vsync)
{
	uint32_t num_present_modes = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &num_present_modes, nullptr), context);

	std::vector<VkPresentModeKHR> present_modes(num_present_modes);

	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &num_present_modes, present_modes.data()), context);

	for (VkPresentModeKHR present_mode : present_modes)
	{
		if (present_mode == VK_PRESENT_MODE_FIFO_KHR && vsync)
			return VK_PRESENT_MODE_FIFO_KHR;
		else if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR && !vsync)
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	return present_modes[0];
}

static VkSwapchainKHR create_swapchain(renderer_context* context, VkSurfaceCapabilitiesKHR surface_caps, uint32_t width, uint32_t height, bool vsync, VkSwapchainKHR old_swapchain)
{
	VkCompositeAlphaFlagBitsKHR surfaceComposite =
		(surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
		: (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
		: (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
		: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	create_info.surface = context->surface;
	create_info.minImageCount = std::max(3u, surface_caps.minImageCount);
	create_info.imageFormat = context->swapchain_format;
	create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	create_info.imageExtent.width = width;
	create_info.imageExtent.height = height;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.queueFamilyIndexCount = 1;
	create_info.pQueueFamilyIndices = &context->graphics_family_index;
	create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	create_info.compositeAlpha = surfaceComposite;
	create_info.presentMode = get_present_mode(context, vsync);
	create_info.oldSwapchain = old_swapchain;


	VkSwapchainKHR swapchain = 0;
	VK_CHECK(vkCreateSwapchainKHR(context->logical_device, &create_info, 0, &swapchain), context);

	return swapchain;
}

swapchain* swapchain_init(renderer_context* context, uint32_t width, uint32_t height, bool use_vsync)
{
	swapchain* out = new swapchain();

	VkSurfaceCapabilitiesKHR surface_caps{};
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &surface_caps), context);

	out->swapchain = create_swapchain(context, surface_caps, width, height, use_vsync, VK_NULL_HANDLE);
	if (out->swapchain == VK_NULL_HANDLE)
		return nullptr;
	out->width = width;
	out->height = height;
	out->vsync = use_vsync;

	uint32_t image_count = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, out->swapchain, &image_count, 0), context);

	std::vector<VkImage> images(image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, out->swapchain, &image_count, images.data()), context);
	out->images = std::move(images);

	//todo(Alex) : image views for the swapchain

	return out;
}

void swapchian_destroy(swapchain* swapchain, renderer_context* context)
{
	if (!swapchain)
		return;

	if (swapchain->swapchain != VK_NULL_HANDLE)
	{
		VK_CHECK(vkDeviceWaitIdle(context->logical_device), context);
		vkDestroySwapchainKHR(context->logical_device, swapchain->swapchain, context->host_allocator);
		swapchain->images.clear();
		swapchain->swapchain = VK_NULL_HANDLE;
	}
	delete swapchain;
}

bool swapchian_update(swapchain* swapchain, renderer_context* context, uint32_t new_width, uint32_t new_height, bool use_vsync)
{
	if (new_width == 0 || new_height == 0)
		return false;

	if (new_width == swapchain->width && new_height == swapchain->height && swapchain->vsync == use_vsync)
		return true;

	VkSurfaceCapabilitiesKHR surface_caps{};
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &surface_caps), context);

	VkSwapchainKHR old_swapchain = swapchain->swapchain;
	swapchain->swapchain = VK_NULL_HANDLE;
	VK_CHECK(vkDeviceWaitIdle(context->logical_device), context);

	swapchain->swapchain = create_swapchain(context, surface_caps, new_width, new_height, use_vsync, old_swapchain);
	if (swapchain->swapchain == VK_NULL_HANDLE)
	{
		swapchain->swapchain = old_swapchain;
		return false;
	}

	swapchain->width = new_width;
	swapchain->height = new_height;
	swapchain->vsync = use_vsync;

	uint32_t image_count = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, swapchain->swapchain, &image_count, 0), context);

	std::vector<VkImage> images(image_count);
	VK_CHECK(vkGetSwapchainImagesKHR(context->logical_device, swapchain->swapchain, &image_count, images.data()), context);
	swapchain->images = std::move(images);

	vkDestroySwapchainKHR(context->logical_device, old_swapchain, context->host_allocator);

	//todo(Alex) : image views for the swapchain

	VK_CHECK(vkDeviceWaitIdle(context->logical_device), context);
	return true;
}