#include "mio.h"
#include <ctype.h>

#define USELUA

#ifdef USELUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
static lua_State *L = NULL;
#endif

#define PS1 "> "
#define COLS 80
#define ROWS 7
#define INPUT 80-2

static char screen[ROWS][COLS+1];
static char *input = screen[ROWS-1] + 2;
static int cursor = 0;

static void scrollup(void)
{
	int i;
	for (i = 1; i < ROWS; i++)
		memcpy(screen[i-1], screen[i], COLS);
	screen[ROWS-1][0] = 0;
}

void console_print(const char *s)
{
	strlcat(screen[ROWS-1], s, COLS);
}

void console_printnl(const char *s)
{
	strlcat(screen[ROWS-1], s, COLS);
	scrollup();
}

#ifdef USELUA
static int ffi_print(lua_State *L)
{
	int i, n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1); /* tostring */
		lua_pushvalue(L, i); /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1); /* get result */
		if (!s) return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) console_print(" ");
		console_print(s);
		lua_pop(L, 1); /* pop result */
	}
	console_printnl("");
	return 0;
}
#endif

void console_init(void)
{
	memset(screen, 0, sizeof screen);

#ifdef USELUA
	L = luaL_newstate();
	luaL_openlibs(L);
	lua_settop(L, 0);
	lua_register(L, "print", ffi_print);
#endif

	console_print(PS1);
}

void console_update(int key, int mod)
{
	char cmd[INPUT];

	if (mod & GLUT_ACTIVE_ALT) return;
	if (mod & GLUT_ACTIVE_CTRL) return;
	if (key >= 0xE000 && key < 0xF900) return; // in PUA
	if (key >= 0x10000) return; // outside BMP

	if (key == '\r') {
		if (input[0] == '=') {
			strlcpy(cmd, "return ", sizeof cmd);
			strlcat(cmd, input + 1, sizeof cmd);
		} else {
			strlcpy(cmd, input, sizeof cmd);
		}
		scrollup();

#ifdef USELUA
		int status;
		status = luaL_dostring(L, cmd);
		if (status && !lua_isnil(L, -1)) {
			const char *msg = lua_tostring(L, -1);
			if (msg == NULL) msg = "(error object is not a string)";
			console_printnl(msg);
			lua_pop(L, 1);
		}
		if (lua_gettop(L) > 0) {
			lua_getglobal(L, "print");
			lua_insert(L, 1);
			lua_pcall(L, lua_gettop(L)-1, 0, 0);
		}
#endif

		console_print(PS1);
		cursor = 0;
	} else if (key == 0x08 || key == 0x7F) {
		if (cursor > 0)
			input[--cursor] = 0;
	} else if (isprint(key) && cursor + 1 < INPUT) {
		input[cursor++] = key;
		input[cursor] = 0;
	}
}

void console_draw(mat4 clip_from_view, mat4 view_from_world, struct font *font, float size)
{
	float model_view[16];
	float screenw = 2 / clip_from_view[0]; // assume we have an orthogonal matrix
	float cellw = font_width(font, size, "0");
	float cellh = size + 2;
	float ascent = size * 0.8;
	float descent = size * 0.2;
	float margin = 8;

	int x0 = (screenw - cellw * COLS) / 2;
	int y0 = margin;
	int x1 = x0 + cellw * COLS;
	int y1 = y0 + cellh * ROWS;
	float x, y;
	int i;

	mat_identity(model_view);

	draw_begin(clip_from_view, view_from_world);
	draw_set_color(0, 0, 0, 0.8);
	draw_rect(x0-margin, y0-margin, x1+margin, y1+margin);
	draw_end();

	text_begin(clip_from_view, view_from_world);
	text_set_color(1, 1, 1, 1);
	text_set_font(font, size);

	y = y0 + ascent;

	for (i = 0; i < ROWS; i++) {
		x = text_show(x0, y, screen[i]);
		y += cellh;
	}
	y -= cellh;

	text_end();

	draw_begin(clip_from_view, view_from_world);
	draw_set_color(1, 1, 1, 1);
	draw_line(x+1, y-ascent, 0, x+1, y+descent-1, 0);
	draw_end();
}
