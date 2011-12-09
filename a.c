#include "mio.h"

#include <time.h>

static struct font *droid_sans;
static struct font *droid_sans_mono;

void sys_hook_init(int argc, char **argv)
{
	fprintf(stderr, "OpenGL %s; ", glGetString(GL_VERSION));
	fprintf(stderr, "GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glClearColor(0.3, 0.3, 0.3, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	droid_sans = load_font("data/fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("data/fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);


	console_init();

	sys_start_idle_loop();
}

void sys_hook_mouse_move(int x, int y, int mod)
{
}

void sys_hook_mouse_down(int x, int y, int btn, int mod)
{
}

void sys_hook_mouse_up(int x, int y, int btn, int mod)
{
}

void sys_hook_key_char(int key, int mod)
{
	if (mod == SYS_MOD_ALT && key == SYS_KEY_F4) {
		exit(0);
	} else if (mod == SYS_MOD_ALT && key == '\r') {
		if (sys_is_fullscreen())
			sys_leave_fullscreen();
		else
			sys_enter_fullscreen();
	} else if (key == 27) {
		exit(0);
	} else {
		console_update(key, mod);
	}
}

void sys_hook_key_down(int key, int mod)
{
}

void sys_hook_key_up(int key, int mod)
{
}

void sys_hook_draw(int width, int height)
{
	float projection[16];
	float model_view[16];

	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	mat_ortho(projection, 0, width, height, 0, -1, 1);
	mat_identity(model_view);

	draw_begin(projection, model_view);
	draw_set_color(0, 0, 1, 1);
	draw_line(10, 10, 0, width - 10, height - 10, 0);
	draw_set_color(1, 1, 0, 0.3);
	draw_rect(5, 5, 200, 150);
	draw_end();

	text_begin(projection);
	text_set_color(1, 0.8, 0.8, 1);
	text_set_font(droid_sans, 20);
	{
		char buf[80];
		time_t now = time(NULL);
		strcpy(buf, ctime(&now));
		buf[strlen(buf)-1] = 0; // zap newline.
		text_show(8, 20+8, "Hello, world!");
		text_show(8, 40+8, buf);
	}
	text_end();

	console_draw(projection, droid_sans_mono, 15);
}
