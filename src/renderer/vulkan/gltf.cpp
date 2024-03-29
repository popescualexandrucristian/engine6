#include <quad_with_vertex_and_index.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

#include <utils.h>

#include <acp_gltf_vulkan.h>

struct gltf_user_context
{
	acp_vulkan::shader* vertex_shader;
	acp_vulkan::shader* fragment_shader;
	acp_vulkan::program* program;
	std::vector<VkCommandPool> commands_pools;
	std::vector<VkCommandBuffer> command_buffers;
	std::vector<VkDescriptorPool> descriptor_pools;

	acp_vulkan::buffer_data	model_vertex_data;
	acp_vulkan::buffer_data	model_index_data;
	uint32_t model_index_count;
	acp_vulkan::image_data	model_color_texture;
	VkImageView				model_color_texture_view;
	VkSampler				model_color_texture_sampler;
	VkDescriptorSet			model_descriptors;

	struct model_vertex
	{
		float uv[2];
		float padding[2];
		float position[3];
		float padding_1;
	};
};

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double)
{
	gltf_user_context* user = reinterpret_cast<gltf_user_context*>(context->user_context.user_data);

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

	VkViewport viewport = { 0, float(context->swapchain->height), float(context->swapchain->width), -float(context->swapchain->height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(context->swapchain->width), uint32_t(context->swapchain->height)} };

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, user->program->pipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &user->model_vertex_data.buffer, &offset);
	vkCmdBindIndexBuffer(command_buffer, user->model_index_data.buffer, offset, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, user->program->pipeline_layout, 0, 1, &user->model_descriptors, 0, nullptr);

	vkCmdDrawIndexed(command_buffer, user->model_index_count, 1, 0, 0, 0);

	acp_vulkan::renderer_end_main_pass(command_buffer, context, false);
	return true;
}

static bool user_init(acp_vulkan::renderer_context* context)
{
	gltf_user_context* user = reinterpret_cast<gltf_user_context*>(context->user_context.user_data);
	for (size_t i = 0; i < context->max_frames; ++i)
	{
		user->commands_pools.push_back(acp_vulkan::commands_pool_crate(context, context->graphics_family_index, "user_command_pools"));
		user->command_buffers.push_back(VK_NULL_HANDLE);
		user->descriptor_pools.push_back(acp_vulkan::descriptor_pool_create(context, 128, "user_descriptor_pools"));
	}

	user->vertex_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/gltf.vert.spv");
	user->fragment_shader = acp_vulkan::shader_init(context->logical_device, context->host_allocator, "./shaders/gltf.frag.spv");

	acp_vulkan::input_attribute_data vertex_shader_input_attributes{};
	vertex_shader_input_attributes.binding = 0;
	vertex_shader_input_attributes.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertex_shader_input_attributes.offsets = {
		offsetof(gltf_user_context::model_vertex, uv),
		offsetof(gltf_user_context::model_vertex, position) };
	vertex_shader_input_attributes.locations = { 0, 1 };
	vertex_shader_input_attributes.stride = sizeof(gltf_user_context::model_vertex);

	user->program = acp_vulkan::graphics_program_init(
		context->logical_device, context->host_allocator,
		{ user->vertex_shader, user->fragment_shader }, { vertex_shader_input_attributes }, 0, true, true, true, 1,
		&context->swapchain_format, context->depth_format, VK_FORMAT_UNDEFINED, "quad_with_texture_pipeline");

	{
		acp_vulkan::gltf_data gltf_model = acp_vulkan::gltf_data_from_file("./models/avocado/avocado.gltf", context->host_allocator);
		char* gltf_buffer_data = nullptr;
		size_t gltf_buffer_data_size = 0;
		load_binary_data("./models/avocado/avocado.bin", &gltf_buffer_data, gltf_buffer_data_size);

		auto read_gltf_data_to_array = [](const acp_vulkan::gltf_data& model, char* buffer_data, size_t accessor, size_t element_size, char* target_array, size_t target_array_stride)
		{
			const acp_vulkan::gltf_data::buffer_view& buffer_view = model.buffer_views.data[model.accesors.data[accessor].buffer_view];
			//const acp_vulkan::gltf_data::buffer& buffer = model.buffers.data[buffer_view.buffer];
			const char* source_data = buffer_data + buffer_view.byte_offset;
			for (size_t ii = 0; ii < model.accesors.data[accessor].count; ++ii)
			{
				const char* source_ptr = source_data + ii * element_size;
				char* dest_ptr = target_array + (target_array_stride * ii);
				memcpy(dest_ptr, source_ptr, element_size);
			}
		};

		std::vector<uint16_t> model_index_data(gltf_model.accesors.data[4].count);
		user->model_index_count = uint32_t(gltf_model.accesors.data[4].count);
		read_gltf_data_to_array(gltf_model, gltf_buffer_data, 4, sizeof(uint16_t), reinterpret_cast<char*>(model_index_data.data()), sizeof(uint16_t));

		std::vector<gltf_user_context::model_vertex> model_interlived_data(gltf_model.accesors.data[0].count);
		//uv
		read_gltf_data_to_array(gltf_model, gltf_buffer_data, 0, sizeof(float) * 2, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, uv), sizeof(gltf_user_context::model_vertex));
		//position
		read_gltf_data_to_array(gltf_model, gltf_buffer_data, 3, sizeof(float) * 3, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, position), sizeof(gltf_user_context::model_vertex));

		user->model_vertex_data = acp_vulkan::upload_data(context, model_interlived_data.data(), uint32_t(gltf_model.accesors.data[0].count), sizeof(gltf_user_context::model_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "avocado_vertex_buffer");
		user->model_index_data = acp_vulkan::upload_data(context, model_index_data.data(), uint32_t(gltf_model.accesors.data[4].count), sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "avocado_index_buffer");

		user->model_color_texture_sampler = acp_vulkan::create_linear_sampler(context, "avocado_color_sampler");

		user->model_descriptors = VK_NULL_HANDLE;
		VkDescriptorSetAllocateInfo descriptor_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptor_info.descriptorPool = user->descriptor_pools[0];
		descriptor_info.descriptorSetCount = 1;
		descriptor_info.pSetLayouts = &user->program->descriptor_layouts[0].first;
		ACP_VK_CHECK(vkAllocateDescriptorSets(context->logical_device, &descriptor_info, &user->model_descriptors), context);

		auto load_texture = [](acp_vulkan::renderer_context* context, const acp_vulkan::gltf_data::string_view texture_name, uint32_t binding, const char* texture_prefix, acp_vulkan::image_data&	texture, VkImageView& texture_view, VkSampler sampler, VkDescriptorSet descriptor) {
			char texture_buffer_path[256];
			sprintf(texture_buffer_path, "%s/%.*s", texture_prefix, int(texture_name.data_length), texture_name.data);
			acp_vulkan::dds_data dds_data = acp_vulkan::dds_data_from_file(texture_buffer_path, context->host_allocator);
			texture = acp_vulkan::upload_image(context, dds_data.image_mip_data, dds_data.image_create_info, texture_buffer_path);
			VkImageViewCreateInfo image_view_info = acp_vulkan::dds_data_create_view_info(&dds_data, texture.image);
			vkCreateImageView(context->logical_device, &image_view_info, context->host_allocator, &texture_view);
			acp_vulkan::dds_data_free(&dds_data, context->host_allocator);

			VkDescriptorImageInfo image_info;
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.sampler = sampler;
			image_info.imageView = texture_view;

			VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.dstSet = descriptor;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.dstBinding = binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.pImageInfo = &image_info;
			vkUpdateDescriptorSets(context->logical_device, 1, &descriptorWrite, 0, nullptr);
		};

		const acp_vulkan::gltf_data::string_view color_texture_name = gltf_model.images.data[gltf_model.materials.data[0].pbr_metallic_roughness.base_color_texture.index].uri;
		load_texture(context, color_texture_name, 0, "models/avocado", user->model_color_texture, user->model_color_texture_view, user->model_color_texture_sampler, user->model_descriptors);

		acp_vulkan::gltf_data_free(&gltf_model, context->host_allocator);
		free_binary_data(gltf_buffer_data);
	}

	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
	gltf_user_context* user = reinterpret_cast<gltf_user_context*>(context->user_context.user_data);

	vkDestroySampler(context->logical_device, user->model_color_texture_sampler, context->host_allocator);

	vkDestroyImageView(context->logical_device, user->model_color_texture_view, context->host_allocator);

	acp_vulkan::image_destroy(context, user->model_color_texture);

	vmaDestroyBuffer(context->gpu_allocator, user->model_vertex_data.buffer, user->model_vertex_data.allocation);
	vmaDestroyBuffer(context->gpu_allocator, user->model_index_data.buffer, user->model_index_data.allocation);

	for (int i = 0; i < user->descriptor_pools.size(); ++i)
		descriptor_pool_destroy(context, user->descriptor_pools[i]);
	user->descriptor_pools.clear();

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

acp_vulkan::renderer_context* init_gltf_loader_render_context()
{
	acp_vulkan::renderer_context::user_context_data user_context{};
	user_context.renderer_init = &user_init;
	user_context.renderer_resize = &user_resize;
	user_context.renderer_shutdown = &user_shutdown;
	user_context.renderer_update = &user_update;
	user_context.user_data = new gltf_user_context();

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