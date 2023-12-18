#include <user_context.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

static bool user_update(acp_vulkan::renderer_context* context, double)
{
	size_t current_frame = context->current_frame % context->max_frames;
	acp_vulkan::frame_sync& sync = context->frame_syncs[current_frame];

	//aquire free image
	ACP_VK_CHECK(vkWaitForFences(context->logical_device, 1, &sync.render_fence, true, 1000000000), context);
	ACP_VK_CHECK(vkResetFences(context->logical_device, 1, &sync.render_fence), context);

	uint32_t next_image_index = acp_vulkan::acquire_next_image(context->swapchain, context, VK_NULL_HANDLE, sync.present_semaphore);

	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);

	ACP_VK_CHECK(vkResetCommandPool(context->logical_device, user->commands_pools[current_frame], 0), context);

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo command_allocate{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_allocate.commandPool = user->commands_pools[current_frame];
	command_allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_allocate.commandBufferCount = 1;

	ACP_VK_CHECK(vkAllocateCommandBuffers(context->logical_device, &command_allocate, &command_buffer), context);

	VkCommandBufferBeginInfo command_begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	command_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	ACP_VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_begin), context);

	VkRenderingAttachmentInfo color_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	color_attachment.imageView = context->swapchain->views[next_image_index];
	color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkClearValue clear_color{};
	clear_color.color.float32[0] = 1.0f;
	clear_color.color.float32[0] = 0.0f;
	clear_color.color.float32[0] = 0.0f;
	clear_color.color.float32[0] = 1.0f;
	color_attachment.clearValue = clear_color;

	VkRenderingAttachmentInfo depth_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depth_attachment.imageView = context->swapchain->depth_views[next_image_index];
	depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkClearValue clear_depth{};
	clear_depth.depthStencil.depth = 1.0f;
	depth_attachment.clearValue = clear_depth;

	VkRenderingInfo pass_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	pass_info.renderArea.extent.width = context->swapchain->width;
	pass_info.renderArea.extent.height = context->swapchain->height;
	pass_info.layerCount = 1;
	pass_info.colorAttachmentCount = 1;
	pass_info.pColorAttachments = &color_attachment;
	pass_info.pDepthAttachment = &depth_attachment;

	VkImageMemoryBarrier2 depth_barreir = acp_vulkan::image_barrier(context->swapchain->depth_images[current_frame].image,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);

	acp_vulkan::push_pipeline_barrier(command_buffer, 0, 0, nullptr, 1, &depth_barreir);

	vkCmdBeginRendering(command_buffer, &pass_info);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, user->program->pipeline);

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &user->vertex_data.buffer, &offset);

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	vkCmdEndRendering(command_buffer);

	VkImageMemoryBarrier2 present_color_barreir = acp_vulkan::image_barrier(context->swapchain->images[current_frame],
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

	acp_vulkan::push_pipeline_barrier(command_buffer, 0, 0, nullptr, 1, &present_color_barreir);

	ACP_VK_CHECK(vkEndCommandBuffer(command_buffer), context);

	//submit

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &sync.present_semaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &sync.render_semaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &command_buffer;

	ACP_VK_CHECK(vkQueueSubmit(context->graphics_queue, 1, &submit, sync.render_fence), context);

	//present

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;

	present_info.pSwapchains = &context->swapchain->swapchain;
	present_info.swapchainCount = 1;

	present_info.pWaitSemaphores = &sync.render_semaphore;
	present_info.waitSemaphoreCount = 1;

	present_info.pImageIndices = &next_image_index;

	context->current_frame++;
	VkResult submit_state = vkQueuePresentKHR(context->graphics_queue, &present_info);
	if (submit_state != VK_SUCCESS)
	{
		acp_vulkan::renderer_resize(context, context->width, context->height);
	}
	return true;
}

static bool user_init(acp_vulkan::renderer_context* context)
{
	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);

	user->vertex_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/from_buffers.vert.spv");
	user->fragment_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/from_buffers.frag.spv");

	acp_vulkan::input_attribute_data vertex_shader_input_attributes{};
	vertex_shader_input_attributes.binding = 0;
	vertex_shader_input_attributes.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertex_shader_input_attributes.offsets = { offsetof(user_data::vertex, position), offsetof(user_data::vertex, color) };
	vertex_shader_input_attributes.locations = { 0, 1 };
	vertex_shader_input_attributes.stride = sizeof(user_data::vertex);

	user->program = acp_vulkan::graphics_program_init(
		context->logical_device, context->host_allocator, 
		{ user->vertex_shader, user->fragment_shader }, { vertex_shader_input_attributes }, 0, true, true, true, 1, 
		&context->swapchain_format, context->depth_format, VK_FORMAT_UNDEFINED);

	for (size_t i = 0; i < context->frame_syncs.size(); ++i)
	{
		user->commands_pools.push_back(acp_vulkan::commands_pool_crate(context));
		user->descriptor_pools.push_back(acp_vulkan::descriptor_pool_create(context, 128));
	}

	user_data::vertex verts_data[3] = {
		{{-0.5,  0.5, 0.0},0.0,{1.0, 0.0, 0.0}},
		{{ 0.5,  0.5, 0.0},0.0,{0.0, 1.0, 0.0}},
		{{ 0.0, -0.5, 0.0},0.0,{0.0, 0.0, 1.0}}
	};

	user->vertex_data = acp_vulkan::upload_mesh(context, verts_data, 3, sizeof(user_data::vertex));
	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);

	vmaDestroyBuffer(context->gpu_allocator, user->vertex_data.buffer, user->vertex_data.allocation);

	for (int i = 0; i < user->descriptor_pools.size(); ++i)
		descriptor_pool_destroy(context, user->descriptor_pools[i]);
	user->descriptor_pools.clear();

	for (int i = 0; i < user->commands_pools.size(); ++i)
		commands_pool_destroy(context, user->commands_pools[i]);
	user->commands_pools.clear();

	acp_vulkan::graphics_program_destroy(context->logical_device, context->host_allocator, user->program);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->fragment_shader);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->vertex_shader);

	delete user;
}

static const acp_vulkan::renderer_context::user_context_data::resize_context user_resize(acp_vulkan::renderer_context* context, uint32_t new_width, uint32_t new_height)
{
	acp_vulkan::renderer_context::user_context_data::resize_context out{
		.width = new_width, 
		.height = new_height, 
		.use_vsync = context->vsync_state, 
		.use_depth = context->depth_state,
	};
	return out;
}

acp_vulkan::renderer_context* init_user_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new user_data();

	acp_vulkan::renderer_context* out = acp_vulkan::renderer_init(initial_width, initial_height, use_vsync, use_depth, user_context);
	return out;
}