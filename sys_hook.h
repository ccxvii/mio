/* System hook callbacks */

enum {
	SYS_MOD_SHIFT = 1,
	SYS_MOD_CTL = 2,
	SYS_MOD_ALT = 4
};

/* Same button numbers as X11 */
enum {
	SYS_BTN_LEFT = 1,
	SYS_BTN_MIDDLE = 2,
	SYS_BTN_RIGHT = 3,
	SYS_BTN_WHEEL_UP = 4,
	SYS_BTN_WHEEL_DOWN = 5
};

/* For special keys we use the same Unicode PUA codes as Apple. */
enum {
	SYS_KEY_SHIFT = 0xF600, SYS_KEY_CTL, SYS_KEY_ALT,
	SYS_KEY_UP = 0xF700,
	SYS_KEY_DOWN = 0xF701,
	SYS_KEY_LEFT = 0xF702,
	SYS_KEY_RIGHT = 0xF703,
	SYS_KEY_F1 = 0xF704, SYS_KEY_F2, SYS_KEY_F3, SYS_KEY_F4,
	SYS_KEY_F5, SYS_KEY_F6, SYS_KEY_F7, SYS_KEY_F8,
	SYS_KEY_F9, SYS_KEY_F10,
	SYS_KEY_INSERT = 0xF727,
	SYS_KEY_DELETE = 0xF728,
	SYS_KEY_HOME = 0xF729,
	SYS_KEY_END = 0xF72B,
	SYS_KEY_PAGEUP = 0xF72C,
	SYS_KEY_PAGEDOWN = 0xF72D
};

void sys_hook_init(int argc, char **argv);
void sys_hook_draw(int screen_width, int screen_height);

void sys_hook_mouse_move(int x, int y, int mod);
void sys_hook_mouse_down(int x, int y, int btn, int mod);
void sys_hook_mouse_up(int x, int y, int btn, int mod);

void sys_hook_key_char(int c, int mod);
void sys_hook_key_down(int c, int mod);
void sys_hook_key_up(int c, int mod);

int sys_is_fullscreen(void);
void sys_enter_fullscreen(void);
void sys_leave_fullscreen(void);

void sys_is_active(void);
void sys_start_idle_loop(void);
void sys_stop_idle_loop(void);
