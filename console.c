#include "mio.h"
#include <ctype.h>

void warn(const char *fmt, ...)
{
	va_list ap;
	char buf[4096];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	console_printnl(buf);
}

#define PS1 "> "
#define PS2 ">>"
#define COLS 80
#define ROWS 23
#define LAST (ROWS-1)
#define INPUT 80-2
#define HISTORY 100
#define TABSTOP 4

static char screen[ROWS][COLS+1] = {{0}};
static int tail = 0;
static char input[INPUT] = {0};
static int cursor = 0;
static int end = 0;

static struct history_entry {
	TAILQ_ENTRY(history_entry) list;
	char s[INPUT];
} *hist_look;

static TAILQ_HEAD(history_list, history_entry) history;

static void scrollup(void)
{
	int i;
	for (i = 1; i < ROWS; i++)
		memcpy(screen[i-1], screen[i], COLS);
	screen[LAST][0] = 0;
	tail = 0;
}

void console_putc(int c)
{
	putchar(c);
	if (c == '\n') {
		scrollup();
	} else if (tail >= COLS) {
		scrollup();
		screen[LAST][tail++] = c;
	} else if (c == '\t') {
		int n = TABSTOP - (tail % TABSTOP);
		while (n--)
			console_putc(' ');
	} else {
		screen[LAST][tail++] = c;
	}
	screen[LAST][tail] = 0;
}

void console_print(const char *s)
{
	while (*s) console_putc(*s++);
}

void console_printnl(const char *s)
{
	while (*s) console_putc(*s++);
	console_putc('\n');
}

void console_printf(const char *fmt, ...)
{
	va_list ap;
	char buf[4096];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	console_print(buf);
}

static void console_history_push(char *s)
{
	if (strlen(s) > 0) {
		struct history_entry *line = malloc(sizeof *line);
		strlcpy(line->s, s, sizeof line->s);
		TAILQ_INSERT_HEAD(&history, line, list);
		hist_look = NULL;
	}
}

static void console_history_prev(void)
{
	struct history_entry *candidate;
	candidate = hist_look ? TAILQ_NEXT(hist_look, list) : TAILQ_FIRST(&history);
	if (candidate) {
		hist_look = candidate;
		memcpy(input, hist_look->s, INPUT);
		cursor = end = strlen(input);
	}
}

static void console_history_next(void)
{
	hist_look = hist_look ? TAILQ_PREV(hist_look, history_list, list) : NULL;
	if (hist_look) {
		memcpy(input, hist_look->s, INPUT);
		cursor = end = strlen(input);
	} else {
		cursor = end = 0;
		input[0] = 0;
	}
}

static void console_enter(void)
{
	char cmd[INPUT];

	console_history_push(input);
	console_print(PS1);
	console_printnl(input);

	if (input[0] == '=') {
		strlcpy(cmd, "return ", sizeof cmd);
		strlcat(cmd, input + 1, sizeof cmd);
	} else {
		strlcpy(cmd, input, sizeof cmd);
	}

	input[0] = 0;
	cursor = end = 0;

	run_string(cmd);
}

static void console_insert(int c)
{
	hist_look = NULL;
	if (end + 1 < INPUT) {
		memmove(input + cursor + 1, input + cursor, end - cursor);
		input[cursor++] = c;
		input[++end] = 0;
	}
}

static void console_backspace(void)
{
	hist_look = NULL;
	if (cursor > 0) {
		memmove(input + cursor - 1, input + cursor, end - cursor);
		input[--end] = 0;
		--cursor;
	}
}

static void console_delete(void)
{
	hist_look = NULL;
	if (cursor < end) {
		memmove(input + cursor, input + cursor + 1, end - cursor);
		input[--end] = 0;
	}
}

void console_backspace_word(void)
{
	while (cursor > 0 && !isalnum(input[cursor-1])) console_backspace();
	while (cursor > 0 && isalnum(input[cursor-1])) console_backspace();
}

void console_delete_word(void)
{
	while (cursor < end && isalnum(input[cursor])) console_delete();
	while (cursor < end && !isalnum(input[cursor])) console_delete();
}

void console_skip_word_left(void)
{
	while (cursor > 0 && !isalnum(input[cursor-1])) --cursor;
	while (cursor > 0 && isalnum(input[cursor-1])) --cursor;
}

void console_skip_word_right(void)
{
	while (cursor < end && !isalnum(input[cursor])) ++cursor;
	while (cursor < end && isalnum(input[cursor])) ++cursor;
}

#define CTL(x) (x-64)

void console_keyboard(int key, int mod)
{
#ifdef __APPLE__
	/* Stupid Apple... */
	if (key == 0x7F) key = 0x08;
	else if (key == 0x8) key = 0x7F;
#endif

	if (key >= 0xE000 && key < 0xF900) return; // in PUA
	if (key >= 0x10000) return; // outside BMP

	if (mod & GLUT_ACTIVE_ALT) {
		switch (key) {
		case 'b': console_skip_word_left(); break;
		case 'f': console_skip_word_right(); break;
		case 'd': console_delete_word();
		}
		return;
	}

	if (mod & GLUT_ACTIVE_CTRL) {
		switch (key) {
		case CTL('A'): cursor = 0; break;
		case CTL('E'): cursor = end; break;
		case CTL('B'): if (cursor > 0) --cursor; break;
		case CTL('F'): if (cursor < end) ++cursor; break;
		case CTL('H'): console_backspace(); break;
		case CTL('U'): while (cursor > 0) console_backspace(); break;
		case CTL('K'): while (cursor < end) console_delete(); break;
		case CTL('W'): console_backspace_word(); break;
		case CTL('T'): if (cursor > 1) { int t = input[cursor-1]; input[cursor-1] = input[cursor]; input[cursor] = t; } break;
		case CTL('P'): console_history_prev(); break;
		case CTL('N'): console_history_next(); break;
		}
		return;
	}

	if (key == '\r') {
		console_enter();
	} else if (key == 0x08) {
		console_backspace();
	} else if (key == 0x7F) {
		console_delete();
	} else if (isprint(key)) {
		console_insert(key);
	}
}

void console_special(int key, int mod)
{
	if (mod & GLUT_ACTIVE_CTRL) {
		switch (key) {
		case GLUT_KEY_LEFT: console_skip_word_left(); break;
		case GLUT_KEY_RIGHT: console_skip_word_right(); break;
		case GLUT_KEY_UP: cursor = 0; break;
		case GLUT_KEY_DOWN: cursor = end; break;
		}
		return;
	}

	switch (key) {
	case GLUT_KEY_LEFT: if (cursor > 0) --cursor; break;
	case GLUT_KEY_RIGHT: if (cursor < end) ++cursor; break;
	case GLUT_KEY_HOME: cursor = 0; break;
	case GLUT_KEY_END: cursor = end; break;
	case GLUT_KEY_UP: console_history_prev(); break;
	case GLUT_KEY_DOWN: console_history_next(); break;
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
	x = text_show(x, y, PS1);
	x = text_show(x, y, input);
	x -= text_width(input + cursor);

	text_end();

	draw_begin(clip_from_view, view_from_world);
	draw_set_color(1, 1, 1, 1);
	draw_line(x+1, y-ascent, 0, x+1, y+descent-1, 0);
	draw_end();
}
