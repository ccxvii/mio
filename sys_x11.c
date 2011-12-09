#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <GL3/gl3w.h>
#include <GL/glx.h>

#include <X11/keysym.h>

#include "sys_hook.h"

static int win_wide = 800;
static int win_high = 600;
static int idle_loop = 0;
static int dirty = 0;

static int glxatts[] =
{
	GLX_RGBA,
	GLX_DOUBLEBUFFER, True, /* Request a double-buffered color buffer with */
	GLX_RED_SIZE, 8, /* the maximum number of bits per component */
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_DEPTH_SIZE, 24,
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

void sys_start_idle_loop(void)
{
	idle_loop = 1;
}

void sys_stop_idle_loop(void)
{
	idle_loop = 0;
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
	switch (keysym) {
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
	return keysym;
}

static int WaitForNotify(Display *dpy, XEvent *event, XPointer arg)
{
	return event->type == MapNotify && event->xmap.window == (Window)arg;
}

static void process_event(XEvent *event)
{
	int x, y, btn, mod;
	KeySym keysym;
	char buf[32];
	int i, len;

	switch (event->type) {
	case Expose:
		break;

	case ConfigureNotify:
		win_wide = event->xconfigure.width;
		win_high = event->xconfigure.height;
		dirty = 1;
		break;

	case KeyPress:
		mod = convert_modifiers(event->xkey.state);
		len = XLookupString(&event->xkey, buf, sizeof buf, &keysym, 0);
		sys_hook_key_down(convert_keysym(keysym), mod);
		for (i = 0; i < len; i++)
			sys_hook_key_char(buf[i], mod);
		dirty = 1;
		break;

	case KeyRelease:
		mod = convert_modifiers(event->xkey.state);
		keysym = XLookupKeysym(&event->xkey, 0);
		sys_hook_key_up(convert_keysym(keysym), mod);
		dirty = 1;
		break;

	case MotionNotify:
		mod = convert_modifiers(event->xbutton.state);
		x = event->xmotion.x;
		y = event->xmotion.y;
		sys_hook_mouse_move(x, y, mod);
		dirty = 1;
		break;

	case ButtonPress:
		mod = convert_modifiers(event->xbutton.state);
		btn = event->xbutton.button;
		x = event->xbutton.x;
		y = event->xbutton.y;
		sys_hook_mouse_down(x, y, btn, mod);
		dirty = 1;
		break;

	case ButtonRelease:
		mod = convert_modifiers(event->xbutton.state);
		btn = event->xbutton.button;
		x = event->xbutton.x;
		y = event->xbutton.y;
		sys_hook_mouse_up(x, y, btn, mod);
		dirty = 1;
		break;
	}
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
	int error;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "glx: cannot connect to X server.\n");
		exit(1);
	}

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	visual = glXChooseVisual(dpy, screen, glxatts);
	if (!visual) {
		fprintf(stderr, "glx: cannot find a matching visual.\n");
		exit(1);
	}

	atts.background_pixel = 0;
	atts.border_pixel = 0;
	atts.colormap = XCreateColormap(dpy, root, visual->visual, AllocNone);
	atts.event_mask = StructureNotifyMask | ExposureMask |
		KeyPressMask | KeyReleaseMask |
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, 800, 600, 0, visual->depth,
			InputOutput, visual->visual, mask, &atts);

	sizehints.min_width = win_wide;
	sizehints.min_height = win_high;
	sizehints.flags = PMinSize;
	XSetNormalHints(dpy, win, &sizehints);
	XSetStandardProperties(dpy, win, argv[0], argv[0], None, NULL, 0, &sizehints);

	context = glXCreateContext(dpy, visual, NULL, True);
	if (!context) {
		fprintf(stderr, "glx: cannot create context.\n");
		exit(1);
	}

	XMapWindow(dpy, win);
	XIfEvent(dpy, &event, WaitForNotify, (XPointer)win);

	glXMakeCurrent(dpy, win, context);

	error = gl3wInit();
	if (error) {
		fprintf(stderr, "cannot init gl3w\n");
		exit(1);
	}

	sys_hook_init(argc, argv);

	while (1) {
		while (XPending(dpy)) {
			XNextEvent(dpy, &event);
			process_event(&event);
		}
		if (!idle_loop && !dirty) {
			XNextEvent(dpy, &event);
			process_event(&event);
		}

		sys_hook_draw(win_wide, win_high);
		glFlush();
		glXSwapBuffers(dpy, win);
		dirty = 0;

#ifndef NDEBUG
		int error = glGetError();
		if (error != GL_NO_ERROR)
			fprintf(stderr, "opengl error: %d\n", error);
#endif
	}

	return 0;
}
