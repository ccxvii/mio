#include "mio.h"

#include "syshook.h"

static char *modelname;
static struct model *model;
static struct font *font;

static int frame = 0;
static int anim = 0;

static int showprog = 1;
static int showbone = 0;
static int showwire = 0;

char *progname = "shader: none";

float dist = 1;
float yaw = 0, pitch = 0;

/* uniforms */

struct program *prog, *statprog, *windprog, *boneprog, *textprog, *flatprog;

float projection[16];
float model_view[16];

float light_dir[3] = { -10, -10, 20 };
float light_dir_mv[3];
float light_ambient[3] = { 0.2, 0.2, 0.2 };
float light_diffuse[3] = { 0.8, 0.8, 0.8 };
float light_specular[3] = { 1.0, 1.0, 1.0 };

float wind_dir[3] = { 1, 0, 0 };
float wind_dir_mv[3];
float wind_phase = 0.0;

void sys_hook_init(int argc, char **argv)
{
	flatprog = compile_shader("shaders/static.vs", "shaders/color.fs");
	textprog = compile_shader("shaders/static.vs", "shaders/text.fs");
	statprog = compile_shader("shaders/static.vs", "shaders/simple.fs");
	windprog = compile_shader("shaders/wind.vs", "shaders/transparent.fs");
	boneprog = compile_shader("shaders/bone.vs", "shaders/specular.fs");
	prog = statprog;

	glClearColor(0.3, 0.3, 0.3, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_CULL_FACE);

	if (argc < 2) {
		fprintf(stderr, "usage: viewer model.iqm\n");
		exit(1);
	}

	font = load_font("data/DroidSans.ttf");
	if (!font)
		exit(1);

	modelname = argv[1];
	model = load_iqm_model(modelname);
	if (!model)
		exit(1);

	dist = 2 + measure_iqm_radius(model);

	sys_start_idle_loop();
}

void sys_hook_draw(int w, int h)
{
	struct event *evt;
	int i;

	while ((evt = sys_read_event()))
	{
		static int isdown = 0;
		static int old_x = 0, old_y = 0;
		if (evt->type == SYS_EVENT_KEY_CHAR)
		{
			if (evt->key == 'w') showwire = !showwire;
			if (evt->key == 'b') showbone = !showbone;
			if (evt->key == 'a') anim++;
			if (evt->key == 'A') anim--;
			if (evt->key == '1') showprog = 1;
			if (evt->key == '2') showprog = 2;
			if (evt->key == '3') showprog = 3;
			if (evt->key == ' ')
				sys_stop_idle_loop();
			if (evt->key == 'r')
				sys_start_idle_loop();
			if (evt->key == '\r' && (evt->mod & SYS_MOD_ALT)) {
				if (sys_is_fullscreen())
					sys_leave_fullscreen();
				else
					sys_enter_fullscreen();
			}
			if (evt->key == 27)
				exit(1);
		}
		if (evt->type == SYS_EVENT_MOUSE_MOVE)
		{
			int dx = evt->x - old_x;
			int dy = evt->y - old_y;
			if (isdown) {
				yaw -= dx * 0.3;
				pitch -= dy * 0.2;
				if (pitch < -85) pitch = -85;
				if (pitch > 85) pitch = 85;
			}
			old_x = evt->x;
			old_y = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_DOWN)
		{
			if (evt->btn == SYS_BTN_LEFT) isdown = 1;
			if (evt->btn == SYS_BTN_WHEEL_UP) dist -= 0.1 * dist;
			if (evt->btn == SYS_BTN_WHEEL_DOWN) dist += 0.1 * dist;
			old_x = evt->x;
			old_y = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_UP)
		{
			if (evt->btn == SYS_BTN_LEFT) isdown = 0;
		}
	}

	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* update animation state */

	animate_iqm_model(model, anim, frame/2, (frame%2)/2.0);
	wind_phase = sinf(frame / 60.0) * measure_iqm_radius(model) * 0.1;

	/* set up transforms */

	mat_perspective(projection, 60, (float) w / h, 0.05, 6000);

	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_vec_mul_n(wind_dir_mv, model_view, wind_dir);
	mat_vec_mul_n(light_dir_mv, model_view, light_dir);
	vec_normalize(light_dir_mv);

	mat_translate(model_view, 0, dist, -dist/3);
	mat_rotate_x(model_view, -pitch);
	mat_rotate_z(model_view, -yaw);

	glPolygonMode(GL_FRONT_AND_BACK, showwire ? GL_LINE : GL_FILL);

	/* draw model */

	switch (showprog) {
	case 1:
		progname = "shader: static";
		prog = statprog;
		glUseProgram(prog->program);
		break;
	case 2:
		progname = "shader: bone";
		prog = boneprog;
		glUseProgram(prog->program);
		break;
	case 3:
		progname = "shader: wind";
		prog = windprog;
		glUseProgram(prog->program);
		glUniform3fv(prog->wind, 1, wind_dir_mv);
		glUniform1f(prog->phase, wind_phase);
		break;
	}

	glUniformMatrix4fv(prog->model_view, 1, 0, model_view);
	glUniformMatrix4fv(prog->projection, 1, 0, projection);

	glUniform3fv(prog->light_dir, 1, light_dir_mv);
	glUniform3fv(prog->light_ambient, 1, light_ambient);
	glUniform3fv(prog->light_diffuse, 1, light_diffuse);
	glUniform3fv(prog->light_specular, 1, light_specular);

	draw_iqm_model(model);

	/* draw grid */

	glUseProgram(flatprog->program);
	glEnable(GL_BLEND);

	glUniformMatrix4fv(flatprog->model_view, 1, 0, model_view);
	glUniformMatrix4fv(flatprog->projection, 1, 0, projection);

	glUniform3fv(flatprog->light_dir, 1, light_dir_mv);
	glUniform3fv(flatprog->light_ambient, 1, light_ambient);
	glUniform3fv(flatprog->light_diffuse, 1, light_diffuse);
	glUniform3fv(flatprog->light_specular, 1, light_specular);

	glVertexAttrib3f(ATT_NORMAL, 0, 0, 1);

	if (showbone) {
		glDisable(GL_DEPTH_TEST);
		draw_iqm_bones(model);
		glEnable(GL_DEPTH_TEST);
	}

	glVertexAttrib4f(ATT_COLOR, 0.8, 0.8, 0.8, 1);
	glBegin(GL_LINES);
	for (i = -4; i <= 4; i++) {
		glVertex3f(i, -4, 0);
		glVertex3f(i, 4, 0);
		glVertex3f(-4, i, 0);
		glVertex3f(4, i, 0);
	}
	glEnd();

	glVertexAttrib4f(ATT_COLOR, 0.4, 0.4, 0.4, 0.75);
	glBegin(GL_QUADS);
	glVertex3f(-4, -4, -0.01f);
	glVertex3f(4, -4, -0.01f);
	glVertex3f(4, 4, -0.01f);
	glVertex3f(-4, 4, -0.01f);
	glEnd();

	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(textprog->program);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	mat_ortho(projection, 0, w, h, 0, -1, 1);
	mat_identity(model_view);

	glUniformMatrix4fv(textprog->model_view, 1, 0, model_view);
	glUniformMatrix4fv(textprog->projection, 1, 0, projection);

	glVertexAttrib3f(ATT_COLOR, 1, 0.8, 0.8);
	{
		char buf[80];
		sprintf(buf, "model: %s", modelname);
		draw_string(font, 20, 8, 20+8, buf);
		sprintf(buf, "animation: %s", get_iqm_animation_name(model, anim));
		draw_string(font, 20, 8, 40+8, buf);
		draw_string(font, 20, 8, 60+8, progname);
	}
	glVertexAttrib3f(ATT_COLOR, 0, 0, 0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	frame++;
}
