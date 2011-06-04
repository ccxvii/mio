#include "mio.h"
#include "syshook.h"

#ifdef USELUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
static lua_State *L = NULL;
#endif

#define PS1 "> "
#define COLS 80
#define ROWS 24
#define INPUT 80-2

static struct font *font = NULL;
static float size, cellw, cellh, ascent, descent, margin;
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
static int print(lua_State *L)
{
	int i, n=lua_gettop(L);
	for (i=1; i<=n; i++) {
		if (i > 1)
			console_print(" ");
		if (lua_isstring(L,i))
			console_print(lua_tostring(L,i));
		else if (lua_isnil(L,i))
			console_print("nil");
		else if (lua_isboolean(L,i))
			console_print(lua_toboolean(L,i) ? "true" : "false");
		else {
			char buf[80];
			sprintf(buf, "%s:%p", luaL_typename(L,i), lua_topointer(L,i));
			console_print(buf);
		}
	}
	console_printnl("");
	return 0;
}
#endif

void init_console(char *fontname, float fontsize)
{
	memset(screen, 0, sizeof screen);
	font = load_font(fontname);
	size = fontsize;
	cellw = measure_string(font, size, "0");
	cellh = size + 2;
	ascent = size * 0.8;
	descent = size * 0.2;
	margin = 8;

#ifdef USELUA
	L = lua_open();
	luaopen_base(L);
	lua_settop(L, 0);
	lua_register(L, "print", print);
#endif

	console_print(PS1);
}

void update_console(int key, int mod)
{
	int status;
	char cmd[INPUT];

	if (mod & SYS_MOD_ALT) return;
	if (mod & SYS_MOD_CTL) return;

	if (key == '\r') {
		strlcpy(cmd, input, sizeof cmd);
		scrollup();

#ifdef USELUA
		status = luaL_dostring(L, cmd);
		if (status && !lua_isnil(L, -1)) {
			const char *msg = lua_tostring(L, -1);
			if (msg == NULL) msg = "(error object is not a string)";
			console_printnl(msg);
			lua_pop(L, 1);
		}
#endif

		console_print(PS1);
		cursor = 0;
	} else if (key == '\b') {
		if (cursor > 0)
			input[--cursor] = 0;
	} else if (cursor + 1 < INPUT) {
		input[cursor++] = key;
		input[cursor] = 0;
	}
}

void draw_console(int screenw, int screenh)
{
	int x0 = (screenw - cellw * COLS) / 2;
	int y0 = margin;
	int x1 = x0 + cellw * COLS;
	int y1 = y0 + cellh * ROWS;
	float x, y;
	int i;

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	glColor4f(0, 0, 0, 0.3); glVertex2f(x0-margin, y0-margin);
	glColor4f(0, 0, 0, 0.8); glVertex2f(x0-margin, y1+margin);
	glColor4f(0, 0, 0, 0.8); glVertex2f(x1+margin, y1+margin);
	glColor4f(0, 0, 0, 0.3); glVertex2f(x1+margin, y0-margin);
	glEnd();

	glEnable(GL_TEXTURE_2D);

	y = y0 + ascent;

	glColor3f(1, 1, 1);
	for (i = 0; i < ROWS; i++) {
		x = draw_string(font, size, x0, y, screen[i]);
		y += cellh;
	}
	y -= cellh;

	glDisable(GL_TEXTURE_2D);

	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glVertex2f(x+1, y-ascent);
	glVertex2f(x+1, y+descent-1);
	glVertex2f(x+2, y+descent-1);
	glVertex2f(x+2, y-ascent);
	glEnd();

	glDisable(GL_BLEND);
}
