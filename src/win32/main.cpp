#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include <version.h>

LRESULT WINAPI window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_PAINT:
		ValidateRgn(hWnd, nullptr);
		return 0;

	case WM_SIZE:
		return 0;

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

	HWND h_wnd = CreateWindow(PROJECT_NAME, title, WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		x, y, width, height, NULL, NULL, h_instance, NULL);

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
	HWND h_wnd = create_window(const_cast<char*>(PROJECT_NAME " " PROJECT_VERSION), 0, 0, 256, 256, PFD_TYPE_RGBA, 0);
	if (h_wnd == NULL)
		exit(1);

#ifdef  ENABLE_DEBUG_CONSOLE
	AllocConsole();
	SetConsoleTitleA(PROJECT_NAME " " PROJECT_VERSION " log");
	typedef struct { char* _ptr; int _cnt; char* _base; int _flag; int _file; int _charbuf; int _bufsiz; char* _tmpfname; } FILE_COMPLETE;
	*(FILE_COMPLETE*)stdout = *(FILE_COMPLETE*)_fdopen(_open_osfhandle(intptr_t(GetStdHandle(STD_OUTPUT_HANDLE)), _O_TEXT), "w");
	*(FILE_COMPLETE*)stderr = *(FILE_COMPLETE*)_fdopen(_open_osfhandle(intptr_t(GetStdHandle(STD_ERROR_HANDLE)), _O_TEXT), "w");
	*(FILE_COMPLETE*)stdin = *(FILE_COMPLETE*)_fdopen(_open_osfhandle(intptr_t(GetStdHandle(STD_INPUT_HANDLE)), _O_TEXT), "r");
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
#endif

	ShowWindow(h_wnd, nShowCmd);
	UpdateWindow(h_wnd);

	printf("logging started for %s\n", PROJECT_NAME " " PROJECT_VERSION);

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
	}

	DestroyWindow(h_wnd);
	return int(msg.wParam);
}