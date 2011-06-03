#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syshook.h"

#define MAXQUEUE 32

static struct event event_queue[MAXQUEUE];
static int event_front = 0;
static int event_count = 0;

static void sys_send_event(struct event *evt)
{
	/* coalesce mouse move events */
	if (event_count > 0 && evt->type == SYS_EVENT_MOUSE_MOVE)
	{
		int last = (event_front + event_count - 1) % MAXQUEUE;
		if (event_queue[last].type == SYS_EVENT_MOUSE_MOVE)
		{
			event_queue[last] = *evt;
			return;
		}
	}

	if (event_count < MAXQUEUE)
	{
		int rear = (event_front + event_count) % MAXQUEUE;
		event_queue[rear] = *evt;
		event_count ++;
	}
	else
	{
		fprintf(stderr, "sys: event queue overflow\n");
	}
}

struct event *sys_read_event(void)
{
	struct event *evt;
	if (event_count == 0)
		return NULL;
	evt = event_queue + event_front;
	event_front ++;
	event_front %= MAXQUEUE;
	event_count--;
	return evt;
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

void sys_hook_mouse_move(int x, int y, int mod)
{
	sys_send_mouse(SYS_EVENT_MOUSE_MOVE, x, y, 0, mod);
}

void sys_hook_mouse_down(int x, int y, int btn, int mod)
{
	sys_send_mouse(SYS_EVENT_MOUSE_DOWN, x, y, btn, mod);
}

void sys_hook_mouse_up(int x, int y, int btn, int mod)
{
	sys_send_mouse(SYS_EVENT_MOUSE_UP, x, y, btn, mod);
}

void sys_hook_key_char(int key, int mod)
{
	sys_send_key(SYS_EVENT_KEY_CHAR, key, mod);
}

void sys_hook_key_down(int key, int mod)
{
	sys_send_key(SYS_EVENT_KEY_DOWN, key, mod);
}

void sys_hook_key_up(int key, int mod)
{
	sys_send_key(SYS_EVENT_KEY_UP, key, mod);
}
