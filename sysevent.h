/* Optional event-queue implementation */

enum {
	SYS_EVENT_KEY_CHAR,
	SYS_EVENT_KEY_DOWN,
	SYS_EVENT_KEY_UP,
	SYS_EVENT_MOUSE_MOVE,
	SYS_EVENT_MOUSE_DOWN,
	SYS_EVENT_MOUSE_UP,
};

struct event {
	int type;
	int btn, x, y;
	int key, mod;
};

struct event *sys_read_event(void);
