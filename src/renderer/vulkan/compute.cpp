#include <compute.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

#include <random>

struct compute_user_data
{
	acp_vulkan::shader* compute_shader;
	acp_vulkan::shader* vertex_shader;
	acp_vulkan::shader* fragment_shader;
	acp_vulkan::program* graphics_program;
	acp_vulkan::program* compute_program;

	std::vector<VkCommandPool> graphics_commands_pools;
	std::vector<VkCommandBuffer> graphics_command_buffers;

	std::vector<VkCommandPool> compute_commands_pools;
	std::vector<VkCommandBuffer> compute_command_buffers;

	std::vector<VkDescriptorPool> graphics_descriptor_pools;
	std::vector<VkDescriptorPool> compute_descriptor_pools;

	struct particle_model_vertex
	{
		float position[3];
		float padding;
	};
	acp_vulkan::buffer_data particle_model_vertex_data;
	acp_vulkan::buffer_data particle_model_index_data;

	struct particle
	{
		float position[2];
		float velocity[2];
		float color[3];
		float padding;
	};
	static constexpr uint32_t PARTICLE_COUNT = 1024;
	std::vector< acp_vulkan::buffer_data> particles;

	struct per_frame_data
	{
		float delta_time;
	};
	std::vector<acp_vulkan::buffer_data> per_frame_data_buffers;

	std::vector< VkDescriptorSet> compute_shader_descriptor_set;
};

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double delta_time)
{
	VkCommandBufferBeginInfo command_begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	command_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	compute_user_data* user = reinterpret_cast<compute_user_data*>(context->user_context.user_data);

	compute_user_data::per_frame_data frame_data{};
	frame_data.delta_time = float(delta_time);
	void* frame_data_gpu_visible = nullptr;
	ACP_VK_CHECK(vmaMapMemory(context->gpu_allocator, user->per_frame_data_buffers[current_frame].allocation, &frame_data_gpu_visible), context);
	memcpy(frame_data_gpu_visible, &frame_data, sizeof(frame_data));
	vmaUnmapMemory(context->gpu_allocator, user->per_frame_data_buffers[current_frame].allocation);

	VkDescriptorBufferInfo buffer_info_frame{};
	buffer_info_frame.buffer = user->per_frame_data_buffers[current_frame].buffer;
	buffer_info_frame.offset = 0;
	buffer_info_frame.range = sizeof(compute_user_data::per_frame_data);
	VkDescriptorBufferInfo buffer_info_current{};
	buffer_info_current.buffer = user->particles[current_frame].buffer;
	buffer_info_current.offset = 0;
	buffer_info_current.range = compute_user_data::PARTICLE_COUNT * sizeof(compute_user_data::particle);
	VkDescriptorBufferInfo buffer_info_prev{};
	size_t last_frame = current_frame > 0 ? current_frame - 1 : context->max_frames - 1;
	buffer_info_prev.buffer = user->particles[last_frame].buffer;
	buffer_info_prev.offset = 0;
	buffer_info_prev.range = compute_user_data::PARTICLE_COUNT * sizeof(compute_user_data::particle);

	VkWriteDescriptorSet descriptor_write_frame{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptor_write_frame.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write_frame.dstSet = user->compute_shader_descriptor_set[current_frame];
	descriptor_write_frame.descriptorCount = 1;
	descriptor_write_frame.dstBinding = 0;
	descriptor_write_frame.pBufferInfo = &buffer_info_frame;

	VkWriteDescriptorSet descriptor_write_prev{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptor_write_prev.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_write_prev.dstSet = user->compute_shader_descriptor_set[current_frame];
	descriptor_write_prev.descriptorCount = 1;
	descriptor_write_prev.dstBinding = 1;
	descriptor_write_prev.pBufferInfo = &buffer_info_prev;

	VkWriteDescriptorSet descriptor_write_current{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptor_write_current.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_write_current.dstSet = user->compute_shader_descriptor_set[current_frame];
	descriptor_write_current.descriptorCount = 1;
	descriptor_write_current.dstBinding = 2;
	descriptor_write_current.pBufferInfo = &buffer_info_current;

	VkWriteDescriptorSet descriptor_write[] = { descriptor_write_frame, descriptor_write_prev, descriptor_write_current};
	vkUpdateDescriptorSets(context->logical_device, 3, descriptor_write, 0, nullptr);

	if (user->compute_command_buffers[current_frame] != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(context->logical_device, user->compute_commands_pools[current_frame], 1, &user->compute_command_buffers[current_frame]);
		ACP_VK_CHECK(vkResetCommandPool(context->logical_device, user->compute_commands_pools[current_frame], 0), context);
		user->compute_command_buffers[current_frame] = VK_NULL_HANDLE;
	}

	VkCommandBuffer compute_command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo compute_command_buffer_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	compute_command_buffer_info.commandPool = user->compute_commands_pools[current_frame];
	compute_command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	compute_command_buffer_info.commandBufferCount = 1;

	ACP_VK_CHECK(vkAllocateCommandBuffers(context->logical_device, &compute_command_buffer_info, &compute_command_buffer), context);
	user->compute_command_buffers[current_frame] = compute_command_buffer;

	ACP_VK_CHECK(vkBeginCommandBuffer(compute_command_buffer, &command_begin_info), context);

	vkCmdBindPipeline(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, user->compute_program->pipeline);
	vkCmdBindDescriptorSets(compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		user->compute_program->pipeline_layout, 0, 1, &user->compute_shader_descriptor_set[current_frame], 0, nullptr);

	uint32_t wave_side = (compute_user_data::PARTICLE_COUNT + 255) / 256;
	vkCmdDispatch(compute_command_buffer, wave_side >> 1, wave_side >> 1, 1);

	ACP_VK_CHECK(vkEndCommandBuffer(compute_command_buffer), context);

	if (user->graphics_command_buffers[current_frame] != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(context->logical_device, user->graphics_commands_pools[current_frame], 1, &user->graphics_command_buffers[current_frame]);
		ACP_VK_CHECK(vkResetCommandPool(context->logical_device, user->graphics_commands_pools[current_frame], 0), context);
		user->graphics_command_buffers[current_frame] = VK_NULL_HANDLE;
	}

	VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo command_allocate{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	command_allocate.commandPool = user->graphics_commands_pools[current_frame];
	command_allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_allocate.commandBufferCount = 1;

	ACP_VK_CHECK(vkAllocateCommandBuffers(context->logical_device, &command_allocate, &command_buffer), context);
	user->graphics_command_buffers[current_frame] = command_buffer;

	ACP_VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_begin_info), context);

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

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, user->graphics_program->pipeline);

	VkDeviceSize offsets[2] = { 0 };
	VkBuffer buffers[2] = { user->particle_model_vertex_data.buffer , user->particles[current_frame].buffer };

	vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, user->particle_model_index_data.buffer, *offsets, VK_INDEX_TYPE_UINT16);

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDrawIndexed(command_buffer, 6, compute_user_data::PARTICLE_COUNT, 0, 0, 0);

	acp_vulkan::renderer_end_main_compute_pass(compute_command_buffer, context);
	acp_vulkan::renderer_end_main_pass(command_buffer, context, true);
	return true;
}

static bool user_init(acp_vulkan::renderer_context* context)
{
	compute_user_data* user = reinterpret_cast<compute_user_data*>(context->user_context.user_data);

	user->vertex_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/particle.vert.spv");
	user->fragment_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/particle.frag.spv");

	acp_vulkan::input_attribute_data particle_model_attributes{};
	particle_model_attributes.binding = 0;
	particle_model_attributes.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	particle_model_attributes.offsets = { offsetof(compute_user_data::particle_model_vertex, position) };
	particle_model_attributes.locations = { 0 };
	particle_model_attributes.stride = sizeof(compute_user_data::particle_model_vertex);

	acp_vulkan::input_attribute_data particle_attributes{};
	particle_attributes.binding = 1;
	particle_attributes.input_rate = VK_VERTEX_INPUT_RATE_INSTANCE;
	particle_attributes.offsets = { offsetof(compute_user_data::particle, position), offsetof(compute_user_data::particle, color)};
	particle_attributes.locations = { 1, 2 };
	particle_attributes.stride = sizeof(compute_user_data::particle);

	user->graphics_program = acp_vulkan::graphics_program_init(
		context->logical_device, context->host_allocator, 
		{ user->vertex_shader, user->fragment_shader }, { particle_model_attributes, particle_attributes}, 0, true, true, true, 1, 
		&context->swapchain_format, context->depth_format, VK_FORMAT_UNDEFINED);

	user->compute_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/particle.comp.spv");

	user->compute_program = acp_vulkan::compute_program_init(context->logical_device, context->host_allocator, user->compute_shader, 0, true);

	for (size_t i = 0; i < context->max_frames; ++i)
	{
		user->graphics_commands_pools.push_back(acp_vulkan::commands_pool_crate(context));
		user->graphics_command_buffers.push_back(VK_NULL_HANDLE);

		user->compute_commands_pools.push_back(acp_vulkan::commands_pool_crate(context));
		user->compute_command_buffers.push_back(VK_NULL_HANDLE);

		user->graphics_descriptor_pools.push_back(acp_vulkan::descriptor_pool_create(context, 128));
		user->compute_descriptor_pools.push_back(acp_vulkan::descriptor_pool_create(context, 128));
	}

	acp_vulkan_os_specific_width_and_height width_and_height = acp_vulkan_os_specific_get_width_and_height();
	float aspect_ratio = float(width_and_height.height) / width_and_height.width;

	compute_user_data::particle_model_vertex verts_data[4] = {
		{.position = {  0.5f, -0.5f / aspect_ratio, 0.0f}},
		{.position = { -0.5f, -0.5f / aspect_ratio, 0.0f}},
		{.position = { -0.5f,  0.5f / aspect_ratio, 0.0f}},
		{.position = {  0.5f,  0.5f / aspect_ratio, 0.0f}}
	};
	uint16_t index_data[6] = { 0,1,2,2,3,0 };
	user->particle_model_vertex_data = acp_vulkan::upload_data(context, verts_data, 4, sizeof(compute_user_data::particle_model_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	user->particle_model_index_data = acp_vulkan::upload_data(context, index_data, 6, sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	for (size_t i = 0; i < context->max_frames; ++i)
	{
		VkDescriptorSetAllocateInfo descriptor_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptor_info.descriptorPool = user->compute_descriptor_pools[i];
		descriptor_info.descriptorSetCount = 1;
		descriptor_info.pSetLayouts = &user->compute_program->descriptor_layouts[0].first;
		VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
		ACP_VK_CHECK(vkAllocateDescriptorSets(context->logical_device, &descriptor_info, &descriptor_set), context);
		user->compute_shader_descriptor_set.push_back(descriptor_set);
	}

	std::vector<compute_user_data::particle> init_particle_data(compute_user_data::PARTICLE_COUNT);

	std::default_random_engine rnd_engine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rnd_dist(0.0f, 1.0f);

	auto normalize_vec2 = [](float* data) {
		float l = sqrtf(data[0] * data[0] + data[1] * data[1]);
		data[0] = data[0] / l;
		data[1] = data[1] / l;
	};

	for (auto& particle : init_particle_data) {
		float r = 0.5f * sqrtf(rnd_dist(rnd_engine));
		float theta = rnd_dist(rnd_engine) * 2.0f * 3.14159265358979323846f;
		float x = r * cosf(theta) * aspect_ratio;
		float y = r * sinf(theta);
		particle.position[0] = x;
		particle.position[1] = y;

		particle.velocity[0] = x * 0.00025f;
		particle.velocity[1] = y * 0.00025f;
		normalize_vec2(particle.velocity);

		particle.color[0] = rnd_dist(rnd_engine);
		particle.color[1] = rnd_dist(rnd_engine);
		particle.color[2] = rnd_dist(rnd_engine);
	}

	for (size_t i = 0; i < context->max_frames; ++i)
	{
		acp_vulkan::buffer_data per_frame_data_buffer{};
		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = sizeof(compute_user_data::per_frame_data);
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo vmaalloc_info = {};
		vmaalloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		ACP_VK_CHECK(vmaCreateBuffer(context->gpu_allocator, &buffer_info, &vmaalloc_info,
			&per_frame_data_buffer.buffer,
			&per_frame_data_buffer.allocation,
			nullptr), context);

		user->per_frame_data_buffers.push_back(per_frame_data_buffer);
	}
	
	for (size_t i = 0; i < context->max_frames; ++i)
		user->particles.push_back(acp_vulkan::upload_data(context, init_particle_data.data(), uint32_t(init_particle_data.size()), sizeof(compute_user_data::particle),
			VkBufferUsageFlagBits(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)));

	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
	compute_user_data* user = reinterpret_cast<compute_user_data*>(context->user_context.user_data);

	vmaDestroyBuffer(context->gpu_allocator, user->particle_model_vertex_data.buffer, user->particle_model_vertex_data.allocation);
	vmaDestroyBuffer(context->gpu_allocator, user->particle_model_index_data.buffer, user->particle_model_index_data.allocation);

	for (auto i : user->per_frame_data_buffers)
		vmaDestroyBuffer(context->gpu_allocator, i.buffer, i.allocation);

	for (auto i : user->particles)
		vmaDestroyBuffer(context->gpu_allocator, i.buffer, i.allocation);

	for (int i = 0; i < user->graphics_descriptor_pools.size(); ++i)
		descriptor_pool_destroy(context, user->graphics_descriptor_pools[i]);
	user->graphics_descriptor_pools.clear();

	for (int i = 0; i < user->compute_descriptor_pools.size(); ++i)
		descriptor_pool_destroy(context, user->compute_descriptor_pools[i]);
	user->compute_descriptor_pools.clear();

	for (int i = 0; i < user->graphics_commands_pools.size(); ++i)
		commands_pool_destroy(context, user->graphics_commands_pools[i]);
	user->graphics_commands_pools.clear();

	for (int i = 0; i < user->compute_commands_pools.size(); ++i)
		commands_pool_destroy(context, user->compute_commands_pools[i]);
	user->compute_commands_pools.clear();

	acp_vulkan::program_destroy(context->logical_device, context->host_allocator, user->graphics_program);
	acp_vulkan::program_destroy(context->logical_device, context->host_allocator, user->compute_program);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->fragment_shader);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->vertex_shader);
	acp_vulkan::shader_destroy(context->logical_device, context->host_allocator, user->compute_shader);

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

acp_vulkan::renderer_context* init_compute_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new compute_user_data();

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