#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include <version.h>

#include <user_context.h>
#include <acp_context/acp_vulkan_context.h>
#include <vulkan/vulkan_win32.h>

constexpr double update_rate{ 1.0 / 60.0 };
static HWND window_handle = NULL;

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

const char* object_type(VkDebugReportObjectTypeEXT s)
{
	switch (s) {
	case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
		return "UNKNOWN";
	case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
		return "INSTANCE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
		return "PHYSICAL_DEVICE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
		return "DEVICE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
		return "QUEUE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
		return "SEMAPHORE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
		return "COMMAND_BUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
		return "FENCE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
		return "DEVICE_MEMORY";
	case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
		return "BUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
		return "IMAGE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
		return "EVENT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
		return "QUERY_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
		return "BUFFER_VIEW";
	case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
		return "IMAGE_VIEW";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
		return "SHADER_MODULE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
		return "PIPELINE_CACHE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
		return "PIPELINE_LAYOUT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
		return "RENDER_PASS";
	case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
		return "PIPELINE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
		return "DESCRIPTOR_SET_LAYOUT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
		return "SAMPLER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
		return "DESCRIPTOR_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
		return "DESCRIPTOR_SET";
	case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
		return "FRAMEBUFFER";
	case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
		return "COMMAND_POOL";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
		return "SURFACE_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
		return "SWAPCHAIN_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT:
		return "DEBUG_REPORT_CALLBACK_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:
		return "DISPLAY_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:
		return "DISPLAY_MODE_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT:
		return "VALIDATION_CACHE_EXT";
	case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT:
		return "SAMPLER_YCBCR_CONVERSION";
	case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT:
		return "DESCRIPTOR_UPDATE_TEMPLATE";
	case VK_DEBUG_REPORT_OBJECT_TYPE_CU_MODULE_NVX_EXT:
		return "CU_MODULE_NVX";
	case VK_DEBUG_REPORT_OBJECT_TYPE_CU_FUNCTION_NVX_EXT:
		return "CU_FUNCTION_NVX";
	case VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT:
		return "ACCELERATION_STRUCTURE_KHR";
	case VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT:
		return "ACCELERATION_STRUCTURE_NV";
	case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA_EXT:
		return "BUFFER_COLLECTION_FUCHSIA";
	default:
		return "UNKNOWN";
	}
}

const char* severity(VkDebugReportFlagsEXT s)
{
	switch (s) {
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT : 
		return "info";
	case VK_DEBUG_REPORT_WARNING_BIT_EXT:
		return "warning";
	case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT :
		return "performace warning";
	case VK_DEBUG_REPORT_ERROR_BIT_EXT:
		return "error";
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
		return "debug";
	default:
		return "unknown";
	}
}

VkBool32 debug_callback(
	VkDebugReportFlagsEXT                       flags,
	VkDebugReportObjectTypeEXT                  objectType,
	uint64_t                                    object,
	size_t                                      location,
	int32_t                                     messageCode,
	const char*,
	const char* pMessage,
	void*)
{
#ifdef ENABLE_DEBUG_CONSOLE
	auto o = object_type(objectType);
	auto s = severity(flags);
	printf("[%s(%d): %s(%Id)]\n%s:%zd\n", s, messageCode, o, object, pMessage, location);
#else
	(void)objectType, flags, messageCode, object, pMessage, location;
#endif
	return VK_FALSE;
}

PFN_vkDebugReportCallbackEXT get_renderer_debug_callback()
{
	return &debug_callback;
}

const char* debug_layers[] =
{
	"VK_LAYER_KHRONOS_validation"
};

const char** get_renderer_debug_layers()
{
#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	return debug_layers;
#else
	return nullptr;
#endif
}

size_t get_renderer_debug_layers_count()
{
#ifdef ENABLE_VULKAN_VALIDATION_LAYERS
	return sizeof(debug_layers) / sizeof(debug_layers[0]);
#else
	return 0;
#endif
}

const char* extensions[] =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef ENABLE_DEBUG_CONSOLE
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
};

const char** get_renderer_extensions()
{
	return extensions;
}

size_t get_renderer_extensions_count()
{
	return sizeof(extensions) / sizeof(extensions[0]);
}

bool get_renderer_device_supports_presentation(VkPhysicalDevice physical_device, uint32_t family_index)
{
	return vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, family_index);
}

VkSurfaceKHR create_renderer_surface(VkInstance instance)
{
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hinstance = GetModuleHandle(0);
	createInfo.hwnd = window_handle;

	VkSurfaceKHR surface = 0;
	if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return surface;
}

void destroy_renderer_surface(VkSurfaceKHR surface, VkInstance instance)
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

static acp_vulkan::renderer_context* render_context = nullptr;

LRESULT WINAPI window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_PAINT:
	{
		ValidateRgn(hWnd, nullptr);
		return 0;
	}

	case WM_SIZE:
	{
		if (!render_context)
			return 0;
		RECT client_rect{};
		GetClientRect(hWnd, &client_rect);
		acp_vulkan::renderer_resize(render_context, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
		return 0;
	}

	case WM_CHAR:
		switch (wParam) {
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND create_window(char* title, int x, int y, int width, int height, BYTE type, DWORD flags)
{
	WNDCLASS window_class{};
	HINSTANCE h_instance = GetModuleHandle(NULL);

	window_class.style = CS_OWNDC;
	window_class.lpfnWndProc = &window_proc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = h_instance;
	window_class.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground = NULL;
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = PROJECT_NAME;

	if (!RegisterClass(&window_class)) {
		MessageBox(NULL, "cannot register window class.", "error", MB_OK);
		return NULL;
	}

	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, FALSE);

	HWND h_wnd = CreateWindow(PROJECT_NAME, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		x, y, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top, NULL, NULL, h_instance, NULL);

	if (!h_wnd) {
		MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.",
			"Error", MB_OK);
		return NULL;
	}

	HDC hdc = GetDC(h_wnd);

	PIXELFORMATDESCRIPTOR pfd{};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
	pfd.iPixelType = type;
	pfd.cColorBits = 32;

	int pf = ChoosePixelFormat(hdc, &pfd);
	if (pf == 0) {
		MessageBox(NULL, "cannot find a suitable pixel format.", "error", MB_OK);
		return 0;
	}

	if (SetPixelFormat(hdc, pf, &pfd) == FALSE) {
		MessageBox(NULL, "cannot set format specified.", "Error", MB_OK);
		return 0;
	}

	DescribePixelFormat(hdc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	ReleaseDC(h_wnd, hdc);

	return h_wnd;
}

int APIENTRY WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nShowCmd)
{
	HWND h_wnd = create_window(const_cast<char*>(PROJECT_NAME " " PROJECT_VERSION), 0, 0, initial_width, initial_height, PFD_TYPE_RGBA, 0);
	if (h_wnd == NULL)
		exit(1);

#ifdef  ENABLE_DEBUG_CONSOLE
	AllocConsole();
	SetConsoleTitleA(PROJECT_NAME " " PROJECT_VERSION " log");
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);

	printf("logging started for %s\n", PROJECT_NAME " " PROJECT_VERSION);
#endif

	ShowWindow(h_wnd, nShowCmd);
	UpdateWindow(h_wnd);
	window_handle = h_wnd;

	render_context = init_user_render_context();
	if (!render_context)
	{
		MessageBox(NULL, "cannot initialize the rendering context.", "error", MB_OK);
		DestroyWindow(h_wnd);
		exit(1);
	}

	LARGE_INTEGER performance_frequency_i{};
	QueryPerformanceFrequency(&performance_frequency_i);
	double performance_frequency = double(performance_frequency_i.QuadPart);

	LARGE_INTEGER last_performance_counter_i{};
	QueryPerformanceCounter(&last_performance_counter_i);
	double last_performance_counter = last_performance_counter_i.QuadPart / performance_frequency;

	MSG msg{};
	bool quit = false;
	while (!quit)
	{
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				quit = true;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		LARGE_INTEGER current_performance_counter_i{};
		QueryPerformanceCounter(&current_performance_counter_i);
		const double current_performance_counter = current_performance_counter_i.QuadPart / performance_frequency;
		const double time_delta = current_performance_counter - last_performance_counter;
		if (time_delta > update_rate)
		{
			last_performance_counter = current_performance_counter;

			renderer_update(render_context, time_delta);
		}
	}

	renderer_shutdown(render_context);
	window_handle = NULL;
	DestroyWindow(h_wnd);
#ifdef  ENABLE_DEBUG_CONSOLE
	system("pause");
#endif
	return int(msg.wParam);
}