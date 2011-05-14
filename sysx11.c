#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <X11/keysym.h>

#include "syshook.h"

static int glxatts[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER, True, /* Request a double-buffered color buffer with */
	GLX_RED_SIZE, 8, /* the maximum number of bits per component	 */
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_DEPTH_SIZE, 16,
	None
};

int sys_is_fullscreen(void)
{
	return 0;
}

void sys_enter_fullscreen(void)
{
}

void sys_leave_fullscreen(void)
{
}

static int convert_modifiers(int state)
{
	int mod = 0;
	if (state & ShiftMask)
		mod |= SYS_MOD_SHIFT;
	if (state & ControlMask)
		mod |= SYS_MOD_CTL;
	if (state & Mod1Mask)
		mod |= SYS_MOD_ALT;
	return mod;
}

static int convert_keysym(KeySym keysym)
{
	switch (keysym)
	{
		case XK_Shift_L: return SYS_KEY_SHIFT;
		case XK_Shift_R: return SYS_KEY_SHIFT;
		case XK_Control_L: return SYS_KEY_CTL;
		case XK_Control_R: return SYS_KEY_CTL;
		case XK_Mode_switch: return SYS_KEY_ALT;
		case XK_Alt_L: return SYS_KEY_ALT;
		case XK_Alt_R: return SYS_KEY_ALT;
		case XK_Meta_L: return SYS_KEY_ALT;
		case XK_Meta_R: return SYS_KEY_ALT;
		case XK_Up: return SYS_KEY_UP;
		case XK_Down: return SYS_KEY_DOWN;
		case XK_Left: return SYS_KEY_LEFT;
		case XK_Right: return SYS_KEY_RIGHT;
		case XK_F1: return SYS_KEY_F1;
		case XK_F2: return SYS_KEY_F2;
		case XK_F3: return SYS_KEY_F3;
		case XK_F4: return SYS_KEY_F4;
		case XK_F5: return SYS_KEY_F5;
		case XK_F6: return SYS_KEY_F6;
		case XK_F7: return SYS_KEY_F7;
		case XK_F8: return SYS_KEY_F8;
		case XK_F9: return SYS_KEY_F9;
		case XK_F10: return SYS_KEY_F10;
		case XK_Insert: return SYS_KEY_INSERT;
		case XK_Delete: return SYS_KEY_DELETE;
		case XK_Home: return SYS_KEY_HOME;
		case XK_End: return SYS_KEY_END;
		case XK_Prior: return SYS_KEY_PAGEUP;
		case XK_Next: return SYS_KEY_PAGEDOWN;
	}
	if (keysym > 255)
		printf("glx: unknown keysym: 0x%04lx\n", (unsigned long)keysym);
	return keysym;
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

static int WaitForNotify(Display *dpy, XEvent *event, XPointer arg)
{
	return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}

int main(int argc, char **argv)
{
	Display *dpy;
	int screen;
	Window root;
	Window win;
	XEvent event;
	XVisualInfo *visual;
	XSetWindowAttributes atts;
	XSizeHints sizehints;
	GLXContext context;
	int mask;

	KeySym keysym;
	char buf[32];
	int i, len;
	int x, y, btn, mod;

	int win_wide = 512;
	int win_high = 360;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL)
	{
		fprintf(stderr, "glx: cannot connect to X server.\n");
		exit(1);
	}

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	visual = glXChooseVisual(dpy, screen, glxatts);
	if (!visual)
	{
		fprintf(stderr, "glx: cannot find a matching visual.\n");
		exit(1);
	}

	atts.background_pixel = 0;
	atts.border_pixel = 0;
	atts.colormap = XCreateColormap(dpy, root, visual->visual, AllocNone);
	atts.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, 1024, 576, 0, visual->depth,
			InputOutput, visual->visual, mask, &atts);

	sizehints.min_width = win_wide;
	sizehints.min_height = win_high;
	sizehints.flags = PMinSize;
	XSetNormalHints(dpy, win, &sizehints);
	XSetStandardProperties(dpy, win, "Untitled", "Untitled", None, NULL, 0, &sizehints);

	context = glXCreateContext(dpy, visual, NULL, True);
	if (!context)
	{
		fprintf(stderr, "glx: cannot create context.\n");
		exit(1);
	}

	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitForNotify, (XPointer)win);

	glXMakeCurrent(dpy, win, context);

	sys_hook_init(argc, argv);

	while (1)
	{
		if (!XPending(dpy))
		{
			int fd = ConnectionNumber(dpy);
			fd_set fds;
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			select(fd + 1, &fds, 0, 0, &tv);
		}

		while (XPending(dpy))
		{
			XNextEvent(dpy, &event);
			switch (event.type)
			{
				case Expose:
					break;

				case ConfigureNotify:
					win_wide = event.xconfigure.width;
					win_high = event.xconfigure.height;
					break;

				case KeyPress:
					mod = convert_modifiers(event.xkey.state);
					len = XLookupString(&event.xkey, buf, sizeof buf, &keysym, 0);
					sys_send_key(SYS_EVENT_KEY_DOWN, convert_keysym(keysym), mod);
					for (i = 0; i < len; i++)
						sys_send_key(SYS_EVENT_KEY_CHAR, buf[i], mod);
					break;

				case KeyRelease:
					mod = convert_modifiers(event.xkey.state);
					keysym = XLookupKeysym(&event.xkey, 0);
					sys_send_key(SYS_EVENT_KEY_UP, convert_keysym(keysym), mod);
					break;

				case MotionNotify:
					mod = convert_modifiers(event.xbutton.state);
					x = event.xmotion.x;
					y = event.xmotion.y;
					sys_send_mouse(SYS_EVENT_MOUSE_MOVE, x, y, 0, mod);
					break;

				case ButtonPress:
					mod = convert_modifiers(event.xbutton.state);
					btn = event.xbutton.button;
					x = event.xbutton.x;
					y = event.xbutton.y;
					sys_send_mouse(SYS_EVENT_MOUSE_DOWN, x, y, btn, mod);
					break;

				case ButtonRelease:
					mod = convert_modifiers(event.xbutton.state);
					btn = event.xbutton.button;
					x = event.xbutton.x;
					y = event.xbutton.y;
					sys_send_mouse(SYS_EVENT_MOUSE_UP, x, y, btn, mod);
					break;
			}
		}

		sys_hook_loop(win_wide, win_high);

		glFlush();
		glXSwapBuffers(dpy, win);
	}

	puts("bye...");

	return 0;
}
