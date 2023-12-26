#include <quad_with_vertex_and_index.h>

#include <acp_program_vulkan.h>
#include <acp_context/acp_vulkan_context.h>
#include <acp_context/acp_vulkan_context_utils.h>
#include <acp_context/acp_vulkan_context_swapchain.h>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_win32.h>

#include <utils.h>

#include <tiny_gltf.h>

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
	acp_vulkan::image_data	model_texture;
	VkImageView				model_texture_view;
	VkSampler				model_texture_sampler;
	VkDescriptorSet			model_descriptors;

	struct model_vertex
	{
		float uv[2];
		float padding[2];
		float normal[3];
		float padding_1;
		float tangent[4];
		float position[3];
		float padding_2;
	};
};

static bool user_update(acp_vulkan::renderer_context* context, size_t current_frame, VkRenderingAttachmentInfo color_attachment, VkRenderingAttachmentInfo depth_attachment, double)
{
#ifdef _WIN32
	ImGui_ImplWin32_NewFrame();
#else
#error Not implemented for this platform.
#endif

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

	ImGui::NewFrame();
	bool show = true;
	ImGui::ShowDemoWindow(&show);
	ImGui::EndFrame();
	ImGui::Render();

	if (ImDrawData* imgui_data = ImGui::GetDrawData(); imgui_data)
	{
		ImGui_ImplVulkan_RenderDrawData(imgui_data, command_buffer);
	}

	acp_vulkan::renderer_end_main_pass(command_buffer, context, false);
	return true;
}

static void init_imgui(acp_vulkan::renderer_context* context)
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 16 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 16 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 16 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 16 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 128;
	pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imgui_pool{ VK_NULL_HANDLE };
	ACP_VK_CHECK(vkCreateDescriptorPool(context->logical_device, &pool_info, context->host_allocator, &imgui_pool),context);
	gltf_user_context* user = reinterpret_cast<gltf_user_context*>(context->user_context.user_data);
	user->descriptor_pools.push_back(imgui_pool);
	//initialize imgui library
	ImGui::CreateContext();

#ifdef _WIN32
	void* main_window_handle = acp_vulkan_os_specific_get_main_window_handle();
	ImGui_ImplWin32_Init(main_window_handle); 
#else
	#error Not implemented for this platform.
#endif

	//TODO(Alex) : Save the settings for imgui in a proper way.
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	size_t font_size = 0;
	uint32_t* font_data = nullptr;
	bool has_font = load_binary_data("./fonts/Cousine-Regular.ttf", &font_data, font_size);
	if (!has_font)
	{
#ifdef ENABLE_DEBUG_CONSOLE
		printf("Unable to load the imgui font\n");
#endif
		abort();
	}
	const ImFontConfig font_config{};
	ImFont* font = io.Fonts->AddFontFromMemoryTTF(font_data, static_cast<int>(font_size * sizeof(uint32_t)), 16.0f);
	if (font)
		io.FontDefault = font;

	size_t frag_size = 0;
	uint32_t* frag_data = nullptr;
	bool has_frag = load_binary_data("./shaders/imgui.frag.spv", &frag_data, frag_size);

	size_t vert_size = 0;
	uint32_t* vert_data = nullptr;
	bool has_vert = load_binary_data("./shaders/imgui.vert.spv", &vert_data, vert_size);

	if (!has_frag || !has_vert)
	{
#ifdef ENABLE_DEBUG_CONSOLE
		printf("Unable to load the imgui shaders\n");
#endif
		abort();
	}

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = context->instance;
	init_info.PhysicalDevice = context->physical_device;
	init_info.Device = context->logical_device;
	init_info.Queue = context->graphics_queue;
	init_info.DescriptorPool = imgui_pool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.UseDynamicRendering = true;
	init_info.ColorAttachmentFormat = context->swapchain_format;
	init_info.DepthAttachmentFormat = context->depth_format;
	init_info.GPUAllocator = context->gpu_allocator;
	init_info.VertData = vert_data;
	init_info.VertSize = vert_size;
	init_info.FragData = frag_data;
	init_info.FragSize = frag_size;

	ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

	_aligned_free(frag_data);
	_aligned_free(vert_data);

	//execute a gpu command to upload imgui font textures
	immediate_submit(context, [&](VkCommandBuffer) {
		ImGui_ImplVulkan_CreateFontsTexture();
		});

	_aligned_free(font_data);
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

	char* model_data = nullptr;
	size_t model_data_size = 0;
	load_binary_data("./models/avocado/avocado.gltf", &model_data, model_data_size);

	{
		tinygltf::Model gltf_model;
		tinygltf::TinyGLTF gltf_loader;
		std::string errors;
		std::string warnings;
		gltf_loader.LoadASCIIFromString(&gltf_model, &errors, &warnings, reinterpret_cast<char*>(model_data), unsigned int(model_data_size), "models/avocado", true);
		_aligned_free(model_data);

		std::vector<uint16_t> model_index_data(gltf_model.accessors[4].count);
		read_gltf_data_to_array(&gltf_model, 4, sizeof(uint16_t), reinterpret_cast<char*>(model_index_data.data()), sizeof(uint16_t));

		std::vector<gltf_user_context::model_vertex> model_interlived_data(gltf_model.accessors[0].count);
		//uv
		read_gltf_data_to_array(&gltf_model, 0, sizeof(float) * 2, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, uv), sizeof(gltf_user_context::model_vertex));
		//normal
		read_gltf_data_to_array(&gltf_model, 1, sizeof(float) * 3, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, normal), sizeof(gltf_user_context::model_vertex));
		//tangent
		read_gltf_data_to_array(&gltf_model, 2, sizeof(float) * 4, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, tangent), sizeof(gltf_user_context::model_vertex));
		//position
		read_gltf_data_to_array(&gltf_model, 3, sizeof(float) * 3, reinterpret_cast<char*>(model_interlived_data.data()) + offsetof(gltf_user_context::model_vertex, position), sizeof(gltf_user_context::model_vertex));

		init_imgui(context);

		user->model_vertex_data = acp_vulkan::upload_data(context, model_interlived_data.data(), uint32_t(gltf_model.accessors[0].count), sizeof(gltf_user_context::model_vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, "avocado_vertex_buffer");
		user->model_index_data = acp_vulkan::upload_data(context, model_interlived_data.data(), uint32_t(gltf_model.accessors[4].count), sizeof(gltf_user_context::model_vertex), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, "avocado_index_buffer");

		//todo textures
		//todo textures views
	}
	user->model_texture_sampler = acp_vulkan::create_linear_sampler(context, "avocado_sampler");

	//todo descriptors

	return true;
}

static void user_shutdown(acp_vulkan::renderer_context* context)
{
#ifdef _WIN32
	ImGui_ImplWin32_Shutdown();
#else
#error Not implemented for this platform.
#endif
	ImGui_ImplVulkan_Shutdown();

	gltf_user_context* user = reinterpret_cast<gltf_user_context*>(context->user_context.user_data);

	vkDestroySampler(context->logical_device,user->model_texture_sampler, context->host_allocator);

	vmaDestroyBuffer(context->gpu_allocator, user->model_vertex_data.buffer, user->model_vertex_data.allocation);
	vmaDestroyBuffer(context->gpu_allocator, user->model_index_data.buffer, user->model_index_data.allocation);

	for (int i = 0; i < user->descriptor_pools.size(); ++i)
		descriptor_pool_destroy(context, user->descriptor_pools[i]);
	user->descriptor_pools.clear();

	for (int i = 0; i < user->commands_pools.size(); ++i)
		commands_pool_destroy(context, user->commands_pools[i]);
	user->commands_pools.clear();

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