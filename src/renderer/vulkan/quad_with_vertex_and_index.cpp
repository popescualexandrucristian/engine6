#include <quad_with_vertex_and_index.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

struct user_data
{
	acp_vulkan::shader* vertex_shader;
	acp_vulkan::shader* fragment_shader;
	acp_vulkan::program* program;
	std::vector<VkCommandPool> commands_pools;
	std::vector<VkCommandBuffer> command_buffers;
	acp_vulkan::buffer_data vertex_data;
	acp_vulkan::buffer_data index_data;

	struct vertex
	{
		float position[3];
		float pedding;
		float color[3];
		float pedding2;
	};
};

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double)
{
	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);
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

	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &user->vertex_data.buffer, &offset);

	vkCmdBindIndexBuffer(command_buffer, user->index_data.buffer, offset, VK_INDEX_TYPE_UINT16);

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

	acp_vulkan::renderer_end_main_pass(command_buffer, context, false);
	return true;
}

static bool user_init(acp_vulkan::renderer_context* context)
{
	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);

	user->vertex_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/quad_vertex_index.vert.spv");
	user->fragment_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/quad_vertex_index.frag.spv");

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

	for (size_t i = 0; i < context->max_frames; ++i)
	{
		user->commands_pools.push_back(acp_vulkan::commands_pool_crate(context));
		user->command_buffers.push_back(VK_NULL_HANDLE);
	}

	user_data::vertex verts_data[4] = {
		{{  0.5f,  -0.5f, 0.0},0.0f,{1.0f, 0.0f, 0.0f}},
		{{ -0.5f,  -0.5f, 0.0},0.0f,{0.0f, 1.0f, 0.0f}},
		{{ -0.5f,   0.5f, 0.0},0.0f,{0.0f, 0.0f, 1.0f}},
		{{  0.5f,   0.5f, 0.0},0.0f,{1.0f, 0.0f, 1.0f}}
	};

	uint16_t index_data[6] = { 0,1,2,2,3,0 };

	user->vertex_data = acp_vulkan::upload_data(context, verts_data, 4, sizeof(user_data::vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	user->index_data = acp_vulkan::upload_data(context, index_data, 6, sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
	user_data* user = reinterpret_cast<user_data*>(context->user_context.user_data);

	vmaDestroyBuffer(context->gpu_allocator, user->vertex_data.buffer, user->vertex_data.allocation);
	vmaDestroyBuffer(context->gpu_allocator, user->index_data.buffer, user->index_data.allocation);

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

acp_vulkan::renderer_context* init_quad_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new user_data();

	acp_vulkan_os_specific_width_and_height width_and_height = acp_vulkan_os_specific_get_width_and_height();
	acp_vulkan::renderer_context* out = acp_vulkan::renderer_init(
		{
			.width = width_and_height.width,
			.height = width_and_height.height,
			.use_vsync = true,
			.use_depth = true,
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