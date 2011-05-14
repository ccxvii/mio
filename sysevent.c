#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syshook.h"

#define MAXQUEUE 32

static struct event g_event_queue[MAXQUEUE];
static int g_event_front = 0;
static int g_event_count = 0;

void sys_send_event(struct event *evt)
{
	/* coalesce mouse move events */
	if (g_event_count > 0 && evt->type == SYS_EVENT_MOUSE_MOVE)
	{
		int last = (g_event_front + g_event_count - 1) % MAXQUEUE;
		if (g_event_queue[last].type == SYS_EVENT_MOUSE_MOVE)
		{
			g_event_queue[last] = *evt;
			return;
		}
	}

	if (g_event_count < MAXQUEUE)
	{
		int rear = (g_event_front + g_event_count) % MAXQUEUE;
		g_event_queue[rear] = *evt;
		g_event_count ++;
	}
	else
	{
		fprintf(stderr, "sys: event queue overflow\n");
	}
}

struct event * sys_read_event(void)
{
	struct event *evt;
	if (g_event_count == 0)
		return NULL;
	evt = g_event_queue + g_event_front;
	g_event_front ++;
	g_event_front %= MAXQUEUE;
	g_event_count--;
	return evt;
}
