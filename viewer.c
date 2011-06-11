#include "mio.h"

#include "syshook.h"

static char *modelname;
static struct model *model;
static struct font *font;

static int frame = 0;
static int anim = 0;

static int showskel = 0;

float sunpos[] = { -500, -500, 800, 1 };
float suncolor[] = { 1, 1, 1, 1 };
float ambientcolor[] = { 0.1, 0.1, 0.1, 1 };
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

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (argc < 2) {
		fprintf(stderr, "usage: viewer model.iqm\n");
		exit(1);
	}

	modelname = argv[1];
	font = load_font("data/DroidSans.ttf");
	model = load_iqm_model(modelname);

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
	glTranslatef(0, 3, -1);
	glRotatef(30, 0, 0, 1);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogcolor);
	glFogf(GL_FOG_START, 1.0f);
	glFogf(GL_FOG_END, 1500.0f);


	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
	glColor3f(0.7, 0.7, 0.7);
	glBegin(GL_LINES);
	for (i = -4; i <= 4; i++) {
		glVertex3f(i, -4, 0);
		glVertex3f(i, 4, 0);
		glVertex3f(-4, i, 0);
		glVertex3f(4, i, 0);
	}
	glEnd();

	animate_iqm_model(model, anim, frame/3);

	glUseProgram(skelprog);
	glEnable(GL_DEPTH_TEST);
	draw_iqm_model(model, skelprog);
	
	if (showskel) {
		glUseProgram(0);
		glDisable(GL_DEPTH_TEST);
		draw_iqm_bones(model);
	}

	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
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
