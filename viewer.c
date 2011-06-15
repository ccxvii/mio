#include "mio.h"

#include "syshook.h"

static char *modelname;
static struct model *model;
static struct font *font;

static int frame = 0;
static int anim = 0;

static int showanim = 1;
static int showskel = 0;

float dist = 1;

float sunpos[] = { -500, -500, 800, 1 };
float fogcolor[4] = { 73.0/255, 149.0/255, 204.0/255, 1 };

unsigned int prog, treeprog, skelprog;

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void sys_hook_init(int argc, char **argv)
{
	prog = compile_shader("common.vs", "common.fs");
	treeprog = compile_shader("tree.vs", "tree.fs");
	skelprog = compile_shader("skel.vs", "common.fs");

	glClearColor(0.3, 0.3, 0.3, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
	dist = 2 + measure_iqm_radius(model);

	sys_start_idle_loop();
}

void sys_hook_draw(int w, int h)
{
	struct event *evt;
	int i;

	while ((evt = sys_read_event()))
	{
		if (evt->type == SYS_EVENT_KEY_CHAR)
		{
			if (evt->key == 'b')
				showskel = !showskel;
			if (evt->key == 'a')
				anim++;
			if (evt->key == 'A')
				anim--;
			if (evt->key == 't')
				showanim = !showanim;
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

	if (showanim)
		glRotatef(-frame/2, 0, 0, 1);

	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_LINES);
	for (i = -4; i <= 4; i++) {
		glVertex3f(i, -4, 0);
		glVertex3f(i, 4, 0);
		glVertex3f(-4, i, 0);
		glVertex3f(4, i, 0);
	}
	glEnd();

	animate_iqm_model(model, anim, frame/4, (frame%4)/4.0);

	glEnable(GL_DEPTH_TEST);
	if (showanim) {
		glUseProgram(skelprog);
	} else {
		glUseProgram(treeprog);
		glMultiTexCoord2f(GL_TEXTURE1, frame, measure_iqm_radius(model) * 0.1);
	}
	draw_iqm_model(model);

	if (showskel) {
		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);
		draw_iqm_bones(model);
	}

	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glColor4f(0.4, 0.4, 0.4, 0.5);
	glBegin(GL_QUADS);
	glVertex3f(-4, 4, -0.01f);
	glVertex3f(4, 4, -0.01f);
	glVertex3f(4, -4, -0.01f);
	glVertex3f(-4, -4, -0.01f);
	glEnd();

	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	{
		char buf[80];
		sprintf(buf, "model: %s", modelname);
		glColor3f(1, 0.8, 0.8);
		draw_string(font, 20, 8, 20+8, buf);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	frame++;
}
