#include "mio.h"

#include <time.h>

static struct font *droid_sans;
static struct font *droid_sans_mono;

static struct model *model;
static struct animation *animation;
static int icon;

static float cam_dist = 4;
static float cam_yaw = 0;
static float cam_pitch = 0;

static int is_left = 0;
static int is_middle = 0;
static int old_x, old_y;

void sys_hook_init(int argc, char **argv)
{
	fprintf(stderr, "OpenGL %s; ", glGetString(GL_VERSION));
	fprintf(stderr, "GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	droid_sans = load_font("data/fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("data/fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);

	icon = load_texture(0, "data/microveget_jungle.png", 1);

	if (argc > 1) {
		model = load_iqm_model(argv[1]);
		animation = load_iqm_animation(argv[1]);
	} else {
		model = load_obj_model("data/fo_s2_spiketree.obj");
		animation = NULL;
	}

	console_init();

	sys_start_idle_loop();
}

void sys_hook_mouse_move(int x, int y, int mod)
{
	int dx = x - old_x;
	int dy = y - old_y;
	if (is_left) {
		cam_yaw -= dx * 0.3;
		cam_pitch -= dy * 0.2;
		if (cam_pitch < -85) cam_pitch = -85;
		if (cam_pitch > 85) cam_pitch = 85;
		if (cam_yaw < 0) cam_yaw += 360;
		if (cam_yaw > 360) cam_yaw -= 360;
	}
	if (is_middle) {
		cam_dist += dy * 0.01 * cam_dist;
	}
	old_x = x;
	old_y = y;
}

void sys_hook_mouse_down(int x, int y, int btn, int mod)
{
	if (btn == 1)
		is_left = 1;
	if (btn == 2)
		is_middle = 1;
	old_x = x;
	old_y = y;
}

void sys_hook_mouse_up(int x, int y, int btn, int mod)
{
	if (btn == 1)
		is_left = 0;
	if (btn == 2)
		is_middle = 0;
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
	} else if (key == '=' || key == '+' || key == ']') {
		cam_dist -= 0.125 * cam_dist;
	} else if (key == '-' || key == '[') {
		cam_dist += 0.125 * cam_dist;
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
	int i;

	glViewport(0, 0, width, height);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)width / height, 0.05, 5000);
	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_translate(model_view, 0, cam_dist, -cam_dist / 3);
	mat_rotate_x(model_view, -cam_pitch);
	mat_rotate_z(model_view, -cam_yaw);

	glEnable(GL_DEPTH_TEST);

	draw_begin(projection, model_view);
	draw_set_color(1, 1, 1, 0.5f);
	for (i = -4; i <= 4; i++) {
		draw_line(i, -4, 0, i, 4, 0);
		draw_line(-4, i, 0, 4, i, 0);
	}
	draw_end();

	mat4 matbuf[MAXBONE];
	struct pose posebuf[MAXBONE];

	if (animation) {
		static int f = 0;
		extract_pose(posebuf, animation, f++ % 100);
		apply_pose2(matbuf, model->bone, posebuf, model->bone_count);
		draw_model_with_pose(model, projection, model_view, matbuf);
	} else {
		draw_model(model, projection, model_view);
	}


	glDisable(GL_DEPTH_TEST);

	if (animation) {
		draw_begin(projection, model_view);

		apply_pose(matbuf, model->bone, model->bind_pose, model->bone_count);
		draw_set_color(1, 0, 0, 0.1);
		draw_skeleton(model->bone, matbuf, model->bone_count);

		apply_pose(matbuf, model->bone, posebuf, model->bone_count);
		draw_set_color(0, 1, 0, 0.1);
		draw_skeleton(model->bone, matbuf, model->bone_count);

		draw_end();
	}

	mat_ortho(projection, 0, width, height, 0, -1, 1);
	mat_identity(model_view);

	text_begin(projection);
	text_set_font(droid_sans, 20);
	{
		char buf[80];
		time_t now = time(NULL);
		strcpy(buf, ctime(&now));
		buf[strlen(buf)-1] = 0; // zap newline.
		text_set_color(1, 1, 1, 1);
		text_show(8, height-104-8, "Hello, world!");
		text_set_color(0.75, 0.75, 0.75, 1);
		text_show(8, height-84-8, "Hello, world!");
		text_set_color(0.5, 0.5, 0.5, 1);
		text_show(8, height-64-8, "Hello, world!");
		text_set_color(0.25, 0.25, 0.25, 1);
		text_show(8, height-44-8, "Hello, world!");
		text_set_color(0, 0, 0, 1);
		text_show(8, height-24-8, "Hello, world!");
		text_set_color(1, 1, 1, 1);
		text_show(8, height-4-8, buf);
	}
	text_end();

	icon_begin(projection);
	icon_set_color(1, 1, 1, 1);
	icon_show(icon, width - 256, height - 256,  width, height, 0, 0, 1, 1);
	icon_end();

	console_draw(projection, droid_sans_mono, 15);
}
