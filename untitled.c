#include "mio.h"

#include "syshook.h"

#define BIRCHES 1000
float birch_pos[BIRCHES * 16];

static struct model *village;
static struct model *tree, *birch;
static struct model *skydome1;
static struct model *skydome2;
static struct model *cute, *caravan;
static struct tile *land;
static struct font *font;

static float camera_dir[3] = { 0, 0, 0 };
static float camera_pos[3] = { 469, 543, 100 };
//static float camera_pos[3] = { 430, 580, 100 };
static float camera_rot[3] = { 0, 0, 0 };
static int action_forward = 0;
static int action_backward = 0;
static int action_left = 0;
static int action_right = 0;

static int old_x = 0, old_y = 0;

static int show_console = 0;

static int prog = 0, treeprog = 0, skelprog = 0;

float sunpos[] = { 0, 0, 800, 1 };
float suncolor[] = { 1, 1, 1, 1 };
float ambientcolor[] = { 0.1, 0.1, 0.1, 1 };
float fogcolor[4] = { 73.0/255, 149.0/255, 204.0/255, 1 };

void perspective(float fov, float aspect, float near, float far)
{
	fov = fov * 3.14159 / 360.0;
	fov = tan(fov) * near;
	glFrustum(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

void rotvec(float a, float b, float c, float *in, float *out)
{
	float xp, yp, zp;
	float x, y, z;
	x = in[0];
	y = in[1];
	z = in[2];

	// - x -
	yp = y * cos(a) - z * sin(a);
	zp = y * sin(a) + z * cos(a);
	y = yp;
	z = zp;

	// - y -
	xp = x * cos(b) + z * sin(b);
	zp = -x * sin(b) + z * cos(b);
	x = xp;
	z = zp;

	// - z -
	xp = x * cos(c) - y * sin(c);
	yp = x * sin(c) + y * cos(c);
	x = xp;
	y = yp;

	out[0] = x;
	out[1] = y;
	out[2] = z;
}

void updatecamera(void)
{
	float dir[3];
	float speed = 0.1;
	rotvec(camera_rot[0]*M_PI/180, camera_rot[1]*M_PI/180, camera_rot[2]*M_PI/180, camera_dir, dir);
	camera_pos[0] += dir[0] * speed;
	camera_pos[1] += dir[1] * speed;
	camera_pos[2] += dir[2] * speed;
}

void sys_hook_init(int argc, char **argv)
{
	float one = 1;
	int i;

	printf("loading data files...\n");

	prog = compile_shader("common.vs", "common.fs");
	treeprog = compile_shader("tree.vs", "tree.fs");
	skelprog = compile_shader("skel.vs", "common.fs");

	init_console("data/DroidSans.ttf", 15);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	glShadeModel(GL_SMOOTH);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, fogcolor);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, &one);

	font = load_font("data/DroidSans.ttf"); if (!font) exit(1);
	village = load_obj_model("data/village/village.obj"); if (!village) exit(1);

	printf("loading terrain\n");
	init_tileset();
	land = load_tile("data/tile_h.png");

	// init_sky_dome(15, 15);
	// skytex = load_texture("data/DuskStormonHorizon.jpg", 0, 0, 0, 0);
	skydome1 = load_obj_model("data/sky/sky_fog_tryker.obj"); if (!skydome1) exit(1);
	skydome2 = load_obj_model("data/sky/canope_tryker.obj"); if (!skydome2) exit(1);

	birch = load_iqm_model("data/vegetation/fo_s2_birch.iqm");

	tree = load_iqm_model("data/vegetation/ju_s2_big_tree.iqm"); if (!tree) exit(1);
	cute = load_iqm_model("data/tr_mo_cute/tr_mo_cute.iqm");
	caravan = load_iqm_model("data/ca_hof/ca_hof.iqm");

	for (i = 0; i < BIRCHES; i++) {
		float *m = &birch_pos[i*16];
		float x = (rand() % 5000) * 0.1 + 300;
		float y = (rand() % 5000) * 0.1 + 300;
		float z = height_at_tile_location(land, x, y);
		float s = 1.0 + sinf((rand() % 628) * 0.01) * 0.3;
		float r = rand() % 360;
		mat_identity(m);
		mat_translate(m, x, y, z);
		mat_rotate_z(m, r);
		mat_scale(m, s, s, s);
	}

	camera_pos[2] = height_at_tile_location(land, camera_pos[0], camera_pos[1]) + 2;

	sys_start_idle_loop();
}

void sys_hook_draw(int w, int h)
{
	struct event *evt;
	static int isdown = 0;

	while ((evt = sys_read_event()))
	{
		if (evt->type == SYS_EVENT_KEY_CHAR)
		{
			if (!show_console) {
				if (evt->key == '`')
					show_console = 1;
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
			} else {
				if (evt->key == 27)
					show_console = 0;
				else
					update_console(evt->key, evt->mod);
			}
		}

		if (!show_console) {
			if (evt->type == SYS_EVENT_KEY_DOWN) {
				switch (evt->key) {
				case SYS_KEY_UP: action_forward = 1; break;
				case SYS_KEY_DOWN: action_backward = 1; break;
				case SYS_KEY_LEFT: action_left = 1; break;
				case SYS_KEY_RIGHT: action_right = 1; break;
				}
			}
			if (evt->type == SYS_EVENT_KEY_UP) {
				switch (evt->key) {
				case SYS_KEY_UP: action_forward = 0; break;
				case SYS_KEY_DOWN: action_backward = 0; break;
				case SYS_KEY_LEFT: action_left = 0; break;
				case SYS_KEY_RIGHT: action_right = 0; break;
				}
			}
		}

		if (evt->type == SYS_EVENT_MOUSE_MOVE)
		{
			int dx = evt->x - old_x;
			int dy = evt->y - old_y;
			if (isdown) {
				camera_rot[2] -= dx * 0.3;
				camera_rot[0] -= dy * 0.2;
				if (camera_rot[0] < -85) camera_rot[0] = -85;
				if (camera_rot[0] > 85) camera_rot[0] = 85;
			}
			old_x = evt->x;
			old_y = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_DOWN)
		{
			if (evt->btn & 1) isdown = 1;
			old_x = evt->x;
			old_y = evt->y;
		}
		if (evt->type == SYS_EVENT_MOUSE_UP)
		{
			if (evt->btn & 1) isdown = 0;
		}
	}

	glViewport(0, 0, w, h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//	glPolygonMode(GL_BACK, GL_LINE);

	static int idx = 0;
	idx ++;

	/*
	 * Update camera
	 */

	if (action_forward) camera_dir[1] = 1;
	else if (action_backward) camera_dir[1] = -1;
	else camera_dir[1] = 0;
	if (action_left) camera_rot[2] += 3;
	if (action_right) camera_rot[2] -= 3;
	updatecamera();

	 /*
	 * Draw rotating models
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspective(60, (float) w / h, 0.05, 6000); /* 5cm to 6km clip planes */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(-90, 1, 0, 0); /* Z-up */
	glRotatef(-camera_rot[1], 0, 1, 0);
	glRotatef(-camera_rot[0], 1, 0, 0);
	glRotatef(-camera_rot[2], 0, 0, 1);
	glTranslatef(-camera_pos[0], -camera_pos[1], -camera_pos[2]);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3f(1, 1, 1);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, sunpos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, suncolor);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogcolor);
	glFogf(GL_FOG_START, 1.0f);
	glFogf(GL_FOG_END, 1500.0f);

	if (caravan) animate_iqm_model(caravan, 0, idx/2, (idx%2)/2.0);
	if (cute) animate_iqm_model(cute, 33, idx/2, (idx%2)/2.0);

	glUseProgram(prog);
	glDisable(GL_CULL_FACE);

	glPushMatrix();
	draw_obj_model(village, 474, 548, height_at_tile_location(land, 474, 548));
	glPopMatrix();

	glUseProgram(skelprog);

	if (caravan) {
		static float caravan_z = 600;
		caravan_z -= 4.0/60.0;
		glPushMatrix();
		glTranslatef(470, caravan_z, height_at_tile_location(land, 470, caravan_z));
		draw_iqm_model(caravan);
		glPopMatrix();
	}
	if (cute) {
		glPushMatrix();
		glTranslatef(469, 550, height_at_tile_location(land, 469, 550));
		draw_iqm_model(cute);
		glPopMatrix();
	}

	glUseProgram(prog);
	glEnable(GL_CULL_FACE);

	draw_tile(land);

	glUseProgram(prog);

	glPushMatrix();
	glTranslatef(0, 0, 150);
	glScalef(5, 5, 5);
	draw_obj_model(skydome2, 0, 0, 0);
	glPopMatrix();

	glUseProgram(0);
	glColor3f(1,1,1);

	glPushMatrix();
	glScalef(20, 20, 20);
	draw_obj_model(skydome1, 0, 0, 0);
	glPopMatrix();

	glDisable(GL_CULL_FACE);

	glUseProgram(treeprog);
//	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);

	glMultiTexCoord2f(GL_TEXTURE6, idx, 1);
	glPushMatrix();
	glTranslatef(490, 590, height_at_tile_location(land, 490, 590));
	draw_iqm_model(tree);
	glPopMatrix();

	glColor3f(1,0,0);
	draw_iqm_instances(birch, birch_pos, BIRCHES);
//	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);

	/*
	* Draw text overlay
	*/

	glUseProgram(0);

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	{
		char buf[80];
		sprintf(buf, "Location: %d %d %d", (int)camera_pos[0], (int)camera_pos[1], (int)camera_pos[2]);
		glColor3f(1, 0.8, 0.8);
		draw_string(font, 24, 8, 24+4, buf);
	}
	glDisable(GL_BLEND);

	if (show_console)
		draw_console(w, h);
}
