#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include <version.h>

#include <triangle.h>
#include <quad_with_vertex_and_index.h>
#include <quad_with_vertex_index_and_texture.h>
#include <dear_imgui.h>
#include <compute.h>
#include <gltf.h>

#include <acp_context/acp_vulkan_context.h>
#include <imgui.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr double update_rate{ 1.0 / 60.0 };
static HWND window_handle = NULL;

void acp_vulkan_os_specific_log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
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

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
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

	HWND h_wnd = CreateWindowEx(0, PROJECT_NAME, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
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
	HWND h_wnd = create_window(const_cast<char*>(PROJECT_NAME " " PROJECT_VERSION), 0, 0, 800, 600, PFD_TYPE_RGBA, 0);
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

#ifdef TRIANGLE_EXAMPLE
	render_context = init_triangle_render_context();
#elif QUAD_EXAMPLE
	render_context = init_quad_render_context();
#elif QUAD_WITH_TEXTURE_EXAMPLE
	render_context = init_quad_with_texture_render_context();
#elif DEAR_IMGUI_EXAMPLE
	render_context = init_imgui_render_context();
#elif COMPUTE_EXAMPLE
	render_context = init_compute_render_context();
#elif GLTF_EXAMPLE
	render_context = init_gltf_loader_render_context();
#else
	#error No example selected please set the define for one of them.
#endif
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