#include <triangle.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

struct triangle_data
{
	acp_vulkan::shader* vertex_shader;
	acp_vulkan::shader* fragment_shader;
	acp_vulkan::program* program;
	std::vector<VkCommandPool> commands_pools;
	std::vector<VkCommandBuffer> command_buffers;
};

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double)
{
	triangle_data* user = reinterpret_cast<triangle_data*>(context->user_context.user_data);
	if (user->command_buffers[current_frame] != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(context->logical_device, user->commands_pools[current_frame], 1, &user->command_buffers[current_frame]);
		ACP_VK_CHECK(vkResetCommandPool(context->logical_device, user->commands_pools[current_frame], 0), context);
		user->command_buffers[current_frame] = VK_NULL_HANDLE;
	}

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo command_allocate{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_allocate.commandPool = user->commands_pools[current_frame];
	command_allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_allocate.commandBufferCount = 1;

	ACP_VK_CHECK(vkAllocateCommandBuffers(context->logical_device, &command_allocate, &command_buffer), context);
	user->command_buffers[current_frame] = command_buffer;

	VkCommandBufferBeginInfo command_begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	command_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	ACP_VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_begin), context);

	VkClearValue clear_color{};
	clear_color.color.float32[0] = 1.0f;
	clear_color.color.float32[0] = 0.0f;
	clear_color.color.float32[0] = 0.0f;
	clear_color.color.float32[0] = 1.0f;
	color_attachment.clearValue = clear_color;

	VkClearValue clear_depth{};
	clear_depth.depthStencil.depth = 1.0f;
	depth_attachment.clearValue = clear_depth;

	acp_vulkan::renderer_start_main_pass(command_buffer, context, color_attachment, depth_attachment);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, user->program->pipeline);

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	acp_vulkan::renderer_end_main_pass(command_buffer, context, false);
	return true;
}

static bool user_init(acp_vulkan::renderer_context* context)
{
	triangle_data* user = reinterpret_cast<triangle_data*>(context->user_context.user_data);

	user->vertex_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/triangle.vert.spv");
	user->fragment_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/triangle.frag.spv");

	user->program = acp_vulkan::graphics_program_init(
		context->logical_device, context->host_allocator, 
		{ user->vertex_shader, user->fragment_shader }, {}, 0, false, false, true, 1, 
		&context->swapchain_format, context->depth_format, VK_FORMAT_UNDEFINED, "triangle_pipeline");

	for (size_t i = 0; i < context->max_frames; ++i)
	{
		user->commands_pools.push_back(acp_vulkan::commands_pool_crate(context, context->graphics_family_index, "user_command_pool"));
		user->command_buffers.push_back(VK_NULL_HANDLE);
	}

	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
	triangle_data* user = reinterpret_cast<triangle_data*>(context->user_context.user_data);

	for (int i = 0; i < user->commands_pools.size(); ++i)
		commands_pool_destroy(context, user->commands_pools[i]);
	user->commands_pools.clear();

	acp_vulkan::program_destroy(context->logical_device, context->host_allocator, user->program);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->fragment_shader);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->vertex_shader);

	delete user;
}

static const acp_vulkan::renderer_context::user_context_data::resize_context user_resize(acp_vulkan::renderer_context* context, uint32_t new_width, uint32_t new_height)
{
	return {
		.width = new_width, 
		.height = new_height, 
		.use_vsync = context->vsync_state, 
		.use_depth = context->depth_state,
	};
}

acp_vulkan::renderer_context* init_triangle_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new triangle_data();

	acp_vulkan_os_specific_width_and_height width_and_height = acp_vulkan_os_specific_get_width_and_height();
	acp_vulkan::renderer_context* out = acp_vulkan::renderer_init(
		{
			.width = width_and_height.width,
			.height = width_and_height.height,
			.use_vsync = false,
			.use_depth = false,
#if defined(ENABLE_DEBUG_CONSOLE) && defined(ENABLE_VULKAN_VALIDATION_LAYERS)
			.use_validation = true,
#endif
#if defined(ENABLE_DEBUG_CONSOLE) && defined(ENABLE_VULKAN_VALIDATION_LAYERS) && defined(ENABLE_VULKAN_VALIDATION_SYNCHRONIZATION_LAYER)
			.use_synchronization_validation = true,
#endif
			.user_context = user_context,
		}
	);
	return out;
}