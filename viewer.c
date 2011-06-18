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

float sunpos[] = { -500, -500, 800, 1 };
float fogcolor[4] = { 73.0/255, 149.0/255, 204.0/255, 1 };

unsigned int prog, windprog, boneprog;

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void sys_hook_init(int argc, char **argv)
{
	prog = compile_shader("common.vs", "common.fs");
	windprog = compile_shader("tree.vs", "tree.fs");
	boneprog = compile_shader("skel.vs", "common.fs");

	glClearColor(0.3, 0.3, 0.3, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 10.0f);
	glFogf(GL_FOG_END, 1000.0f);
	glFogfv(GL_FOG_COLOR, fogcolor);

	if (argc < 2) {
		fprintf(stderr, "usage: viewer model.iqm\n");
		exit(1);
	}

	modelname = argv[1];
	font = load_font("data/DroidSans.ttf");
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspective(60, (float) w / h, 0.05, 6000); /* 5cm to 6km clip planes */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(-90, 1, 0, 0); /* Z-up */
	glTranslatef(0, dist, -dist/3);

	glLightfv(GL_LIGHT0, GL_POSITION, sunpos);

	glRotatef(-pitch, 1, 0, 0);
	glRotatef(-yaw, 0, 0, 1);

	glPolygonMode(GL_FRONT_AND_BACK, showwire ? GL_LINE : GL_FILL);

	glUseProgram(0);
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_LINES);
	for (i = -4; i <= 4; i++) {
		glVertex3f(i, -4, 0);
		glVertex3f(i, 4, 0);
		glVertex3f(-4, i, 0);
		glVertex3f(4, i, 0);
	}
	glEnd();

	animate_iqm_model(model, anim, frame/2, (frame%2)/2.0);

	switch (showprog) {
	case 1:
		progname = "shader: static";
		glUseProgram(prog);
		break;
	case 2:
		progname = "shader: bone";
		glUseProgram(boneprog);
		break;
	case 3:
		progname = "shader: wind";
		glUseProgram(windprog);
		glMultiTexCoord2f(GL_TEXTURE6, frame, measure_iqm_radius(model) * 0.1);
		break;
	default:
		glUseProgram(0);
		break;
	}

	draw_iqm_model(model);

	if (showbone) {
		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);
		draw_iqm_bones(model);
		glEnable(GL_DEPTH_TEST);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUseProgram(0);
	glEnable(GL_BLEND);

	glColor4f(0.4, 0.4, 0.4, 0.5);
	glBegin(GL_QUADS);
	glVertex3f(-4, 4, -0.01f);
	glVertex3f(4, 4, -0.01f);
	glVertex3f(4, -4, -0.01f);
	glVertex3f(-4, -4, -0.01f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	{
		char buf[80];
		sprintf(buf, "model: %s", modelname);
		glColor3f(1, 0.8, 0.8);
		draw_string(font, 20, 8, 20+8, buf);
		sprintf(buf, "animation: %s", get_iqm_animation_name(model, anim));
		glColor3f(1, 0.8, 0.8);
		draw_string(font, 20, 8, 40+8, buf);
		draw_string(font, 20, 8, 60+8, progname);
	}
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glDisable(GL_BLEND);

	frame++;
}
