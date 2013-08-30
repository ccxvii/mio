#include "mio.h"

extern struct scene *scene;

static int screenw = 800, screenh = 600;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;
static int lasttime = 0;

static int showconsole = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static float cam_dist = 5;
static float cam_yaw = 0;
static float cam_pitch = -20;

void togglefullscreen(void)
{
	static int oldw = 100, oldh = 100, oldx = 0, oldy = 0;
	static int isfullscreen = 0;
	if (!isfullscreen) {
		oldw = glutGet(GLUT_WINDOW_WIDTH);
		oldh = glutGet(GLUT_WINDOW_HEIGHT);
		oldx = glutGet(GLUT_WINDOW_X);
		oldy = glutGet(GLUT_WINDOW_Y);
		glutFullScreen();
	} else {
		glutPositionWindow(oldx, oldy);
		glutReshapeWindow(oldw, oldh);
	}
	isfullscreen = !isfullscreen;
}

static void mouse(int button, int state, int x, int y)
{
	state = state == GLUT_DOWN;
	if (button == GLUT_LEFT_BUTTON) mouseleft = state;
	if (button == GLUT_MIDDLE_BUTTON) mousemiddle = state;
	if (button == GLUT_RIGHT_BUTTON) mouseright = state;
	mousex = x;
	mousey = y;
	glutPostRedisplay();
}

static void motion(int x, int y)
{
	int dx = x - mousex;
	int dy = y - mousey;
	if (mouseleft) {
		cam_yaw -= dx * 0.3;
		cam_pitch -= dy * 0.2;
		if (cam_pitch < -85) cam_pitch = -85;
		if (cam_pitch > 85) cam_pitch = 85;
		if (cam_yaw < 0) cam_yaw += 360;
		if (cam_yaw > 360) cam_yaw -= 360;
	}
	if (mousemiddle || mouseright) {
		cam_dist += dy * 0.01 * cam_dist;
		if (cam_dist < 0.1) cam_dist = 0.1;
		if (cam_dist > 100) cam_dist = 100;
	}
	mousex = x;
	mousey = y;
	glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y)
{
	int mod = glutGetModifiers();
	if ((mod & GLUT_ACTIVE_ALT) && key == '\r') {
		togglefullscreen();
	} else if (showconsole) {
		if (key == '`' || key == 27)
			showconsole = !showconsole;
		else
			console_keyboard(key, mod);
	} else {
		if (key == 27)
			exit(0);
		if (key == '`' || key == '\r')
			showconsole = !showconsole;
	}

	glutPostRedisplay();
}

static void special(int key, int x, int y)
{
	int mod = glutGetModifiers();
	if (key == GLUT_KEY_F4 && mod == GLUT_ACTIVE_ALT)
		exit(0);
	if (showconsole)
		console_special(key, mod);
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	screenw = w;
	screenh = h;
	glViewport(0, 0, w, h);
	render_reshape(w, h);
}

static void display(void)
{
	float projection[16];
	float view[16];

	int thistime, timediff;

	thistime = glutGet(GLUT_ELAPSED_TIME);
	timediff = thistime - lasttime;
	lasttime = thistime;

	// update world

	// update_scene(scene, timediff * 0.001);

	// draw world

	mat_perspective(projection, 75, (float)screenw / screenh, 0.1, 1000);
	mat_identity(view);
	mat_rotate_x(view, -90);

	mat_translate(view, 0, cam_dist, -cam_dist / 5);
	mat_rotate_x(view, -cam_pitch);
	mat_rotate_z(view, -cam_yaw);

	render_camera(projection, view);

	render_geometry_pass();
	run_function("draw_geometry");

	render_light_pass();
	run_function("draw_light");

	render_forward_pass();
	render_sky();
	render_finish();

	// draw ui

	glViewport(0, 0, screenw, screenh);
	mat_ortho(projection, 0, screenw, screenh, 0, -1, 1);
	mat_identity(view);

	// copy forward buffer to screen
	render_blit(projection, view, screenw, screenh);
	//render_debug_buffers(projection, view);

	if (showconsole)
		console_draw(projection, view, droid_sans_mono, 15);

	glutSwapBuffers();

	glutPostRedisplay();

	gl_assert("swap buffers");
}

int main(int argc, char **argv)
{
	int i;

	glutInit(&argc, argv);
	glutInitContextVersion(3, 0);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowPosition(50, 50+24);
	glutInitWindowSize(screenw, screenh);
	glutInitDisplayMode(GLUT_SRGB | GLUT_DOUBLE | GLUT_3_2_CORE_PROFILE);
	glutCreateWindow("Mio");

	gl3wInit();

	warn("OpenGL %s; GLSL %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	register_directory("data/");
	register_directory("data/textures/");

	droid_sans = load_font("fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);

	init_lua();
	run_string("require 'strict'");
	run_file("proxy.lua");

	for (i = 1; i < argc; i++)
		run_file(argv[i]);

	glutMainLoop();
	return 0;
}
