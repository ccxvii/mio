#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <GL/gl.h>

#include "syshook.h"

static HWND		g_hwnd = NULL;
static HDC		g_hdc = NULL;
static HGLRC	g_hrc = NULL;
static int		g_capture = 0;

static int g_width = 640;
static int g_height = 480;
static int g_fullscreen = 0;
static WINDOWPLACEMENT g_savedplace;

static void sys_win_error(char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	MessageBox(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}

int sys_is_fullscreen(void)
{
	return g_fullscreen;
}

void sys_enter_fullscreen(void)
{
	if (g_fullscreen)
		return;
	GetWindowPlacement(g_hwnd, &g_savedplace);
	SetWindowLong(g_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
	SetWindowPos(g_hwnd, NULL, 0,0,0,0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	ShowWindow(g_hwnd, SW_SHOWMAXIMIZED);
	g_fullscreen = 1;
}

void sys_leave_fullscreen(void)
{
	if (!g_fullscreen)
		return;
	SetWindowLong(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
	SetWindowPos(g_hwnd, NULL, 0,0,0,0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	SetWindowPlacement(g_hwnd, &g_savedplace);
	g_fullscreen = 0;
}

static int sys_win_enable_opengl(void)
{
	PIXELFORMATDESCRIPTOR pfd;
	int fmt;

	g_hdc = GetDC(g_hwnd);
	if (!g_hdc)
	{
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
	if (fmt == 0)
	{
		sys_win_error("Cannot choose pixel format.");
		return 1;
	}

	if (!SetPixelFormat(g_hdc, fmt, &pfd))
	{
		sys_win_error("Cannot set pixel format.");
		return 1;
	}

	g_hrc = wglCreateContext(g_hdc);
	if (!g_hrc)
	{
		sys_win_error("Cannot create OpenGL context.");
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
		return 1;
	}

	if (!wglMakeCurrent(g_hdc, g_hrc))
	{
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

static void sys_send_mouse(int type, int x, int y, int btn, int mod)
{
	struct event evt;
	evt.type = type;
	evt.x = x;
	evt.y = y;
	evt.btn = btn;
	evt.key = 0;
	evt.mod = mod;
	sys_send_event(&evt);
}

static void sys_send_key(int type, int key, int mod)
{
	struct event evt;
	evt.type = type;
	evt.x = 0;
	evt.y = 0;
	evt.btn = 0;
	evt.key = key;
	evt.mod = mod;
	sys_send_event(&evt);
}

static int convert_keycode(int code)
{
	switch (code)
	{
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

	switch (message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
			if (wparam == SIZE_MINIMIZED)
				return 0;
			g_width = LOWORD(lparam);
			g_height = HIWORD(lparam);
			break;
		case WM_GETMINMAXINFO:
			minmax->ptMinTrackSize.x = 400;
			minmax->ptMinTrackSize.y = 300;
			break;
		case WM_ERASEBKGND:
			return 1; /* we don't need to rease to redraw cleanly */

		case WM_PAINT:
			sys_hook_loop(g_width, g_height);
			SwapBuffers(g_hdc);
			return 0;

		case WM_LBUTTONDOWN:
			if (!g_capture)
				SetCapture(g_hwnd);
			g_capture ++;
			sys_send_mouse(SYS_EVENT_MOUSE_DOWN, x, y, 1, mod);
			return 0;
		case WM_RBUTTONDOWN:
			if (!g_capture)
				SetCapture(g_hwnd);
			g_capture ++;
			sys_send_mouse(SYS_EVENT_MOUSE_DOWN, x, y, 2, mod);
			return 0;
		case WM_MBUTTONDOWN:
			if (!g_capture)
				SetCapture(g_hwnd);
			g_capture ++;
			sys_send_mouse(SYS_EVENT_MOUSE_DOWN, x, y, 3, mod);
			return 0;

		case WM_LBUTTONUP:
			g_capture --;
			if (!g_capture)
				ReleaseCapture();
			sys_send_mouse(SYS_EVENT_MOUSE_UP, x, y, 1, mod);
			return 0;
		case WM_RBUTTONUP:
			g_capture --;
			if (!g_capture)
				ReleaseCapture();
			sys_send_mouse(SYS_EVENT_MOUSE_UP, x, y, 2, mod);
			return 0;
		case WM_MBUTTONUP:
			g_capture --;
			if (!g_capture)
				ReleaseCapture();
			sys_send_mouse(SYS_EVENT_MOUSE_UP, x, y, 3, mod);
			return 0;

		case WM_MOUSEMOVE:
			sys_send_mouse(SYS_EVENT_MOUSE_MOVE, x, y, 0, mod);
			return 0;

		case WM_CHAR:
			sys_send_key(SYS_EVENT_KEY_CHAR, wparam, mod);
			return 0;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if (!(lparam & (1<<30))) /* previous key state */
			{
				key = convert_keycode(wparam);
				if (mod & SYS_MOD_ALT)
					sys_send_key(SYS_EVENT_KEY_CHAR, key, mod);
				sys_send_key(SYS_EVENT_KEY_DOWN, key, mod);
			}
			return 0;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			key = convert_keycode(wparam);
			sys_send_key(SYS_EVENT_KEY_UP, key, mod);
			return 0;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

#ifndef NDEBUG
int main(int argc, char **argv)
#else
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmdstr, int cmdshow)
#endif
{
#ifndef NDEBUG
	HINSTANCE instance = GetModuleHandle(NULL);
#endif
	WNDCLASS wc;
	MSG msg;
	int error;

	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = sys_win_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GLWindow";
	if (!RegisterClass(&wc))
	{
		sys_win_error("Cannot register window class.");
		return 1;
	}

	g_hwnd = CreateWindow("GLWindow", "Untitled",
			WS_OVERLAPPEDWINDOW,
			0, 0, g_width, g_height,
			0, 0, instance, 0);
	if (g_hwnd == NULL)
	{
		sys_win_error("Cannot create main window.");
		return 1;
	}

	ShowWindow(g_hwnd, SW_SHOWNORMAL);

	error = sys_win_enable_opengl();
	if (error)
		return 1;

	sys_hook_init();

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			InvalidateRect(g_hwnd, NULL, 0);
		}
	}

	sys_win_disable_opengl();

	return 0;
}
