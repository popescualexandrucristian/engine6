#include <user_context.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double)
{
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

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &user->vertex_data.buffer, &offset);

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);

	acp_vulkan::renderer_end_main_pass(command_buffer, context);
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
		{{  0.5,  -0.5, 0.0},0.0,{1.0, 0.0, 0.0}},
		{{ -0.5,  -0.5, 0.0},0.0,{0.0, 1.0, 0.0}},
		{{  0.0,   0.5, 0.0},0.0,{0.0, 0.0, 1.0}}
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
	return {
		.width = new_width, 
		.height = new_height, 
		.use_vsync = context->vsync_state, 
		.use_depth = context->depth_state,
	};
}

acp_vulkan::renderer_context* init_user_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new user_data();

	acp_vulkan::renderer_context* out = acp_vulkan::renderer_init(
		{
			.width = initial_width,
			.height = initial_height,
			.use_vsync = use_vsync,
			.use_depth = use_depth,
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