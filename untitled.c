#include "mio.h"

static struct md2_model *model;
static struct font *font;

static int imagetex, imagew, imageh;

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void sys_hook_init(int argc, char **argv)
{
	float light0pos[] = { -50.0, 10.0, -80.0, 1.0 };
	float light0col[] = { 1.0, 1.0, 1.0, 1.0 };
	float light1pos[] = { 0.0, 30.0, -80.0, 1.0 };
	float light1col[] = { 1.0, 0.5, 0.5, 1.0 };

	printf("preparing open gl\n");

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0col);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.02);

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light1pos);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1col);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.02);

	font = load_font("data/CharterBT-Roman.ttf");
	model = md2_load_model("data/tr_mo_kami_caster.md2");

	// imagetex = load_png("PngSuite/f00n2c08.png", &imagew, &imageh);
}

void display(int w, int h)
{
	glViewport(0, 0, w, h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float angle = 0.0;
	static float lerp = 0.0;
	static int idx = 0;
	angle += 1.0;
	if (angle > 360)
		angle = 0;
	lerp += 25.0/60.0;
	if (lerp > 1.0)
	{
		lerp = 0.0;
		idx ++;
	}
	if (idx == md2_get_frame_count(model) - 1)
		idx = 0;

	 /*
	 * Draw rotating models
	 */

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspective(60.0, (float) w / h, 1.0, 5000.0);
	glTranslatef(0, -20, 0);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glTranslatef(-20, 0, -100);
	glRotatef(angle, 0, 1, 0);
	glRotatef(-90, 1, 0, 0);
	glRotatef(-90, 0, 0, 1);

	glColor3f(1, 1, 1);
	md2_draw_frame_lerp(model, 0, idx, idx + 1, lerp);

	glLoadIdentity();
	glTranslatef(+20, 0, -100);
	glRotatef(-angle/5, 0, 1, 0);
	glRotatef(-90, 1, 0, 0);
	glRotatef(-90, 0, 0, 1);

	md2_draw_frame_lerp(model, 0, idx, idx + 1, lerp);

	glDisable(GL_TEXTURE_2D);
	//glDisable(GL_LIGHTING);
	//glEnable(GL_BLEND);
	glLoadIdentity();
	glTranslatef(0, 0, -100);
	glScalef(10, 10, 10);
	glRotatef(10, 0, 1, 0);
	glBegin(GL_QUADS);
	{
		int row, col;
		for (row = -10; row < 10; row++)
		{
			for (col = -10; col < 10; col++)
			{
				if ((row + col) & 1)
					glColor4f(0.6, 0.6, 0.6, 1);
				else
					glColor4f(1.0, 1.0, 1.0, 1);

				glNormal3f(0, 1, 0);
				glVertex3f(row + 0, 0, col + 0);
				glVertex3f(row + 1, 0, col + 0);
				glVertex3f(row + 1, 0, col + 1);
				glVertex3f(row + 0, 0, col + 1);
			}
		}
	}
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
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
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
	glBindTexture(GL_TEXTURE_2D, imagetex);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(10, 50);
	glTexCoord2f(1, 0); glVertex2f(10 + imagew, 50);
	glTexCoord2f(1, 1); glVertex2f(10 + imagew, 50 + imageh);
	glTexCoord2f(0, 1); glVertex2f(10, 50 + imageh);
	glEnd();
	#endif

}

void sys_hook_loop(int width, int height)
{
	struct event *evt;
	static int mousex = 0, mousey = 0, mousez = 0;

	while ((evt = sys_read_event()))
	{
		printf("EVENT %d x=%d y=%d b=%d key=U+%04X mod=0x%02x\n",
				evt->type, evt->x, evt->y, evt->btn, evt->key, evt->mod);
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

	display(width, height);
}

void sys_hook_key(int key)
{
	printf("typed key '%c'\n", key);
}

void sys_hook_mouse(int x, int y, int button, int modifiers, int state)
{
	printf("mouse event %d %d button=%d mod=%d state=%d\n", x, y, button, modifiers, state);
}
