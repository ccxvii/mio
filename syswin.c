#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#include <GL/gl.h>

#include "syshook.h"

static HWND	g_hwnd = NULL;
static HDC	g_hdc = NULL;
static HGLRC	g_hrc = NULL;

static int capture = 0;
static int width = 640;
static int height = 480;
static int fullscreen = 0;
static int active = 0;
static int idle_loop = 0;
static int dirty = 0;
static WINDOWPLACEMENT savedplace;

static void sys_win_error(char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	MessageBox(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}

int sys_is_fullscreen(void)
{
	return fullscreen;
}

void sys_enter_fullscreen(void)
{
	if (fullscreen)
		return;
	GetWindowPlacement(g_hwnd, &savedplace);
	SetWindowLong(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
	SetWindowPos(g_hwnd, NULL, 0,0,0,0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	ShowWindow(g_hwnd, SW_SHOWMAXIMIZED);
	fullscreen = 1;
}

void sys_leave_fullscreen(void)
{
	if (!fullscreen)
		return;
	SetWindowLong(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
	SetWindowPos(g_hwnd, NULL, 0,0,0,0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	SetWindowPlacement(g_hwnd, &savedplace);
	fullscreen = 0;
}

void sys_start_idle_loop(void)
{
	idle_loop = 1;
}

void sys_stop_idle_loop(void)
{
	idle_loop = 0;
}

static int sys_win_enable_opengl(void)
{
	PIXELFORMATDESCRIPTOR pfd;
	int fmt;

	g_hdc = GetDC(g_hwnd);
	if (!g_hdc) {
		sys_win_error("Cannot get device context.");
		return 1;
	}

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	fmt = ChoosePixelFormat(g_hdc, &pfd);
	if (fmt == 0) {
		sys_win_error("Cannot choose pixel format.");
		return 1;
	}

	if (!SetPixelFormat(g_hdc, fmt, &pfd)) {
		sys_win_error("Cannot set pixel format.");
		return 1;
	}

	g_hrc = wglCreateContext(g_hdc);
	if (!g_hrc) {
		sys_win_error("Cannot create OpenGL context.");
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
		return 1;
	}

	if (!wglMakeCurrent(g_hdc, g_hrc)) {
		sys_win_error("Cannot make current OpenGL context.");
		wglDeleteContext(g_hrc);
		ReleaseDC(g_hwnd, g_hdc);
		g_hrc = NULL;
		g_hdc = NULL;
		return 1;
	}

	return 0;
}

static void sys_win_disable_opengl(void)
{
	if (!wglMakeCurrent(NULL, NULL))
		sys_win_error("Cannot release OpenGL and device contexts.");

	wglDeleteContext(g_hrc);
	g_hrc = NULL;

	ReleaseDC(g_hwnd, g_hdc);
	g_hdc = NULL;
}

static int convert_keycode(int code)
{
	switch (code) {
		case VK_SHIFT: return SYS_KEY_SHIFT;
		case VK_CONTROL: return SYS_KEY_CTL;
		case VK_MENU: return SYS_KEY_ALT;
		case VK_UP: return SYS_KEY_UP;
		case VK_DOWN: return SYS_KEY_DOWN;
		case VK_LEFT: return SYS_KEY_LEFT;
		case VK_RIGHT: return SYS_KEY_RIGHT;
		case VK_INSERT: return SYS_KEY_INSERT;
		case VK_DELETE: return SYS_KEY_DELETE;
		case VK_HOME: return SYS_KEY_HOME;
		case VK_END: return SYS_KEY_END;
		case VK_PRIOR: return SYS_KEY_PAGEUP;
		case VK_NEXT: return SYS_KEY_PAGEDOWN;
		case VK_F1: case VK_F2: case VK_F3: case VK_F4:
		case VK_F5: case VK_F6: case VK_F7: case VK_F8:
		case VK_F9: case VK_F10: return code - VK_F1 + SYS_KEY_F1;
	}
	return code;
}

static LRESULT CALLBACK sys_win_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	MINMAXINFO *minmax = (MINMAXINFO*)lparam;
	int x = (signed short) LOWORD(lparam);
	int y = (signed short) HIWORD(lparam);
	int mod;
	int key;

	mod = 0;
	if (GetKeyState(VK_SHIFT) < 0)
		mod |= SYS_MOD_SHIFT;
	if (GetKeyState(VK_CONTROL) < 0)
		mod |= SYS_MOD_CTL;
	if (GetKeyState(VK_MENU) < 0)
		mod |= SYS_MOD_ALT;

	dirty = 1;

	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
			if (wparam == SIZE_MINIMIZED)
				return 0;
			width = LOWORD(lparam);
			height = HIWORD(lparam);
			break;
		case WM_GETMINMAXINFO:
			minmax->ptMinTrackSize.x = 400;
			minmax->ptMinTrackSize.y = 300;
			break;
		case WM_ERASEBKGND:
			return 1; /* we don't need to rease to redraw cleanly */

		case WM_ACTIVATE:
			if (!HIWORD(wparam))
				active = 1;
			else
				active = 0;
			return 0;

		case WM_LBUTTONDOWN:
			if (!capture)
				SetCapture(g_hwnd);
			capture ++;
			sys_hook_mouse_down(x, y, 1, mod);
			return 0;
		case WM_MBUTTONDOWN:
			if (!capture)
				SetCapture(g_hwnd);
			capture ++;
			sys_hook_mouse_down(x, y, 2, mod);
			return 0;
		case WM_RBUTTONDOWN:
			if (!capture)
				SetCapture(g_hwnd);
			capture ++;
			sys_hook_mouse_down(x, y, 3, mod);
			return 0;

		case WM_LBUTTONUP:
			capture --;
			if (!capture)
				ReleaseCapture();
			sys_hook_mouse_up(x, y, 1, mod);
			return 0;
		case WM_MBUTTONUP:
			capture --;
			if (!capture)
				ReleaseCapture();
			sys_hook_mouse_up(x, y, 2, mod);
			return 0;
		case WM_RBUTTONUP:
			capture --;
			if (!capture)
				ReleaseCapture();
			sys_hook_mouse_up(x, y, 3, mod);
			return 0;

		case WM_MOUSEMOVE:
			sys_hook_mouse_move(x, y, mod);
			return 0;

		case WM_CHAR:
			sys_hook_key_char(wparam, mod);
			return 0;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if (!(lparam & (1<<30))) /* previous key state */ {
				key = convert_keycode(wparam);
				if (mod & SYS_MOD_ALT)
					sys_hook_key_char(key, mod);
				sys_hook_key_down(key, mod);
			}
			return 0;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			key = convert_keycode(wparam);
			sys_hook_key_up(key, mod);
			return 0;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

#ifndef NDEBUG
int main(int argc, char **argv)
{
	HINSTANCE instance = GetModuleHandle(NULL);
#else
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmdstr, int cmdshow)
{
	int argc;
	char **argv = CommandLineToArgvA(GetCommandLineA(), &argc);
#endif
	WNDCLASS wc;
	MSG msg;
	int error, okay;

	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = sys_win_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MioWindow";
	if (!RegisterClass(&wc)) {
		sys_win_error("Cannot register window class.");
		return 1;
	}

	g_hwnd = CreateWindow("MioWindow", argv[0],
			WS_OVERLAPPEDWINDOW,
			0, 0, width, height,
			0, 0, instance, 0);
	if (g_hwnd == NULL) {
		sys_win_error("Cannot create main window.");
		return 1;
	}

	ShowWindow(g_hwnd, SW_SHOWNORMAL);

	error = sys_win_enable_opengl();
	if (error)
		return 1;

	sys_hook_init(0, 0);

	while (1) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				goto quit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (!idle_loop && !dirty) {
			if (GetMessage(&msg, NULL, 0, 0)) {
				if (msg.message == WM_QUIT)
					goto quit;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		sys_hook_draw(width, height);
		SwapBuffers(g_hdc);
		dirty = 0;
	}

quit:
	sys_win_disable_opengl();

	return 0;
}
