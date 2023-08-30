#include <utils.h>
#include <context_internal.h>
#include <vma/vk_mem_alloc.h>

VkCommandPool commands_pool_crate(renderer_context* renderer_context)
{
	VkCommandPoolCreateInfo vkCommandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	vkCommandPoolCreateInfo.queueFamilyIndex = renderer_context->graphics_family_index;

	VkCommandPool out = VK_NULL_HANDLE;
	VK_CHECK(vkCreateCommandPool(renderer_context->logical_device, &vkCommandPoolCreateInfo, renderer_context->host_allocator, &out), renderer_context);
	return out;
}

void commands_pool_destroy(renderer_context* renderer_context, VkCommandPool commands_pool)
{
	if (commands_pool == VK_NULL_HANDLE)
		return;

	vkResetCommandPool(renderer_context->logical_device, commands_pool, 0);

	vkDestroyCommandPool(renderer_context->logical_device, commands_pool, renderer_context->host_allocator);
}

VkSemaphore semaphore_create(renderer_context* renderer_context)
{
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore = 0;
	VK_CHECK(vkCreateSemaphore(renderer_context->logical_device, &createInfo, 0, &semaphore), renderer_context);

	return semaphore;
}

void semaphore_destroy(renderer_context* renderer_context, VkSemaphore semaphore)
{
	if (semaphore == VK_NULL_HANDLE)
		return;

	vkDestroySemaphore(renderer_context->logical_device, semaphore, renderer_context->host_allocator);
}

VkDescriptorPool descriptor_pool_create(renderer_context* renderer_context, uint32_t max_descriptor_count)
{
	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, max_descriptor_count },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_descriptor_count },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, max_descriptor_count },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, max_descriptor_count },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_descriptor_count },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_descriptor_count },
	};
	info.maxSets = max_descriptor_count;

	info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
	info.pPoolSizes = pool_sizes;

	VkDescriptorPool out = VK_NULL_HANDLE;
	VK_CHECK(vkCreateDescriptorPool(renderer_context->logical_device, &info, renderer_context->host_allocator, &out), renderer_context);
	return out;
}

void descriptor_pool_destroy(renderer_context* renderer_context, VkDescriptorPool descriptor_pool)
{
	if (descriptor_pool == VK_NULL_HANDLE)
		return;

	vkDestroyDescriptorPool(renderer_context->logical_device, descriptor_pool, renderer_context->host_allocator);
}

VkImageView image_view_create(renderer_context* renderer_context, VkImage image, VkFormat format, uint32_t mip_level, uint32_t level_count, VkImageAspectFlags aspectMask)
{
	VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = mip_level;
	createInfo.subresourceRange.levelCount = level_count;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView view = 0;
	VK_CHECK(vkCreateImageView(renderer_context->logical_device, &createInfo, renderer_context->host_allocator, &view), renderer_context);

	return view;
}

void image_view_destroy(renderer_context* renderer_context, VkImageView image_view)
{
	vkDestroyImageView(renderer_context->logical_device, image_view, renderer_context->host_allocator);
}

image_data image_create(renderer_context* renderer_context, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memory_usage)
{
	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = { width, height, 1 };
	info.mipLevels = mipLevels;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usage;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	image_data image = { VK_NULL_HANDLE, nullptr };

	VmaAllocationCreateInfo allocation_info{};
	allocation_info.flags = memory_usage;
	allocation_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	VK_CHECK(vmaCreateImage(renderer_context->gpu_allocator, &info, &allocation_info, &image.image, &image.memory_allocation, nullptr),renderer_context);
	
	return image;
}

void image_destroy(renderer_context* renderer_context, image_data image_data)
{
	vmaDestroyImage(renderer_context->gpu_allocator, image_data.image, image_data.memory_allocation);
}

VkFence fence_create(renderer_context* renderer_context, bool create_signaled)
{
	VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (create_signaled)
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence out = VK_NULL_HANDLE;
	VK_CHECK(vkCreateFence(renderer_context->logical_device, &info, renderer_context->host_allocator, &out), renderer_context);

	return out;
}

void fence_destroy(renderer_context* renderer_context, VkFence fence)
{
	vkDestroyFence(renderer_context->logical_device, fence, renderer_context->host_allocator);
}


VkImageMemoryBarrier2 image_barrier(VkImage image, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount)
{
	VkImageMemoryBarrier2 result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

	result.srcStageMask = srcStageMask;
	result.srcAccessMask = srcAccessMask;
	result.dstStageMask = dstStageMask;
	result.dstAccessMask = dstAccessMask;
	result.oldLayout = oldLayout;
	result.newLayout = newLayout;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.image = image;
	result.subresourceRange.aspectMask = aspectMask;
	result.subresourceRange.baseMipLevel = baseMipLevel;
	result.subresourceRange.levelCount = levelCount;
	result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return result;
}

void push_pipeline_barrier(VkCommandBuffer commandBuffer, VkDependencyFlags dependencyFlags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, size_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers)
{
	VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = dependencyFlags;
	dependencyInfo.bufferMemoryBarrierCount = unsigned(bufferBarrierCount);
	dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
	dependencyInfo.imageMemoryBarrierCount = unsigned(imageBarrierCount);
	dependencyInfo.pImageMemoryBarriers = imageBarriers;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}