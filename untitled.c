#include "mio.h"

#include "syshook.h"
#include "sysevent.h"

static struct md2_model *model1;
static struct model *model2;
static struct model *model3;
static struct model *model4;
static struct model *skydome1;
static struct model *skydome2;
static struct font *font;
static int grasstex[15];

float light0pos[] = { -5, -5, 10, 1 };
float light0col[] = { 1, 1, 1, 1 };
float light1pos[] = { 0, 0, 1, 1 };
float light1col[] = { 1, 0.8, 0.2, 1 };
float light2pos[] = { 0, 0, 1, 0 }; /* directional */
float light2col[] = { 0.3, 0.3, 0.5, 1 };
float ambientcol[] = { 0, 0, 0, 1 };

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void sys_hook_init(int argc, char **argv)
{
	char buf[100];
	int i;

	printf("loading data files...\n");

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glShadeModel(GL_SMOOTH);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcol);

	font = load_font("data/CharterBT-Roman.ttf");
	model1 = md2_load_model("data/soldier.md2");
	//model2 = load_obj_model("data/ca_hom/ca_hom_armor01.obj");
	//model2 = load_obj_model("data/tr_mo_kami_caster/tr_mo_kami_caster.obj");
	model2 = load_obj_model("data/tr_mo_cute/tr_mo_cute.obj");
	load_obj_animation(model2, "data/tr_mo_cute/tr_mo_cute_idle_occupation1.pc2");
	model3 = load_obj_model("data/ca_hom/ca_hom_armor01.obj");
	model4 = load_obj_model("data/village/village.obj");

	printf("loading terrain textures\n");
	for (i = 0; i < nelem(grasstex); i++) {
		sprintf(buf, "data/terrain/y-bassesiles-256-a-%02d.png", i+1);
		grasstex[i] = load_texture(buf, NULL, NULL, NULL, 0);
	}

	// init_sky_dome(15, 15);
	// skytex = load_texture("data/DuskStormonHorizon.jpg", 0, 0, 0, 0);
	skydome1 = load_obj_model("data/sky/sky_fog_tryker.obj");
	skydome2 = load_obj_model("data/sky/canope_tryker.obj");

	printf("all done\n");
}

void sys_hook_draw(int w, int h)
{
	struct event *evt;
	static int mousex = 0, mousey = 0, mousez = 0;

	while ((evt = sys_read_event()))
	{
		if (evt->type == SYS_EVENT_KEY_CHAR)
		{
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
				exit(0);
		}
		if (evt->type == SYS_EVENT_MOUSE_MOVE)
		{
			mousex = evt->x;
			mousey = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_DOWN)
		{
			if (evt->btn == 5)
				mousez++;
			else if (evt->btn == 4)
				mousez--;
		}
	}

	glViewport(0, 0, w, h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float angle = 0.0;
	static float lerp = 0.0;
	static int idx = 0;
	angle += 1.0;
	if (angle > 360)
		angle = 0;
	lerp += 15.0/60.0;
	if (lerp > 1.0)
	{
		lerp = 0.0;
		idx ++;
	}
	if (idx == 261) idx = 0;
	if (idx == md2_get_frame_count(model1) - 1)
		idx = 0;

	 /*
	 * Draw rotating models
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspective(60, (float) w / h, 0.05, 600); /* 5cm to 600m clip planes */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(-90, 1, 0, 0); /* Z-up */
	glTranslatef(0, 5, -1.5); /* 3 meters back and 1.5 meters up */
	glRotatef(angle, 0, 0, 1); /* spin me right round */

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0col);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0 / 10);

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light1pos);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1col);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1.0 / 5); /* drop off at 5m */

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslatef(-1, 0, 0);
	md2_draw_frame_lerp(model1, 0, idx, idx + 1, lerp);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1, 0, 0);
	glColor3f(1, 1, 1);
	draw_obj_model_frame(model2, idx);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 1, 0);
	draw_obj_model(model3);
	glPopMatrix();

	glPushMatrix();
	draw_obj_model(model4);
	glPopMatrix();

	glPushMatrix();
	glScalef(2, 2, 1);
	{
		int row, col, tex = 0;
		for (row = -40; row <= 40; row++)
		{
			for (col = -40; col <= 40; col++)
			{
				glBindTexture(GL_TEXTURE_2D, grasstex[tex]);
				glBegin(GL_QUADS);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(row + 0, col + 0, 0);
				glTexCoord2f(1, 0); glVertex3f(row + 1, col + 0, 0);
				glTexCoord2f(1, 1); glVertex3f(row + 1, col + 1, 0);
				glTexCoord2f(0, 1); glVertex3f(row + 0, col + 1, 0);
				glEnd();
				tex = (tex + 1) % nelem(grasstex);
			}
		}
	}
	glPopMatrix();

	glColor3f(1, 1, 1);
	glEnable(GL_BLEND);

	glEnable(GL_LIGHT2);
	glLightfv(GL_LIGHT2, GL_POSITION, light2pos);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light2col);
	draw_obj_model(skydome2);
	glDisable(GL_LIGHT2);

	glDisable(GL_LIGHTING);
	glPushMatrix();
	glScalef(2, 2, 2);
	draw_obj_model(skydome1);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	/* Draw x-y-z-axes */
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4f(1, 0, 0, 1);
	glVertex3f(0, 0, 0); glVertex3f(1, 0, 0);
	glColor4f(0, 1, 0, 1);
	glVertex3f(0, 0, 0); glVertex3f(0, 1, 0);
	glColor4f(0, 0, 1, 1);
	glVertex3f(0, 0, 0); glVertex3f(0, 0, 1);
	glEnd();

	/*
	* Draw text overlay
	*/

	char status[100];

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor3f(1, 0.70, 0.7);
	float x;
	sprintf(status, "Angle: %g", angle);
	draw_string(font, 20, 20, 30, status);
	sprintf(status, "Frame: %d", idx);
	x = measure_string(font, 20, status);
	draw_string(font, 20, w - x - 20, 30, status);

	#if 0
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(10, 50);
	glTexCoord2f(0, 1); glVertex2f(10, 50 + 512);
	glTexCoord2f(1, 1); glVertex2f(10 + 512, 50 + 512);
	glTexCoord2f(1, 0); glVertex2f(10 + 512, 50);
	glEnd();
	#endif

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}
