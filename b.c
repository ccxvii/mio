#include "mio.h"

static int screenw = 800, screenh = 600;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;

static int showconsole = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static struct model *model;

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
		cam_dist -= dy * 0.01 * cam_dist;
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
	if ((mod & GLUT_ACTIVE_ALT) && key == '\r')
		togglefullscreen();
	else if (key ==	'`')
		showconsole = !showconsole;
	else if (showconsole)
		console_update(key, mod);
	else switch (key) {
		case 27: case 'q': exit(0); break;
		case 'f': togglefullscreen(); break;
	}

	glutPostRedisplay();
}

static void special(int key, int x, int y)
{
	if (key == GLUT_KEY_F4 && glutGetModifiers() == GLUT_ACTIVE_ALT)
		exit(0);
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	screenw = w;
	screenh = h;
	glViewport(0, 0, w, h);
}

static void display(void)
{
	float projection[16];
	float model_view[16];
	int i;

	/* Render world */

	glViewport(0, 0, screenw, screenh);

	glClearColor(0.05, 0.05, 0.05, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)screenw / screenh, 0.1, 1000);
	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_translate(model_view, 0, cam_dist, -cam_dist / 5);
	mat_rotate_x(model_view, -cam_pitch);
	mat_rotate_z(model_view, -cam_yaw);

	render_geometry_pass();
	{
		render_model(model, projection, model_view);
	}

	render_light_pass();
	{
		static const vec3 sun_direction = { 0, 0, 1 };
		static const vec3 sun_color = { 1, 1, 1 };

		render_sun_light(projection, model_view, sun_direction, sun_color);

		static const vec3 moon_direction = { 1, 0, 0 };
		static const vec3 moon_color = { 0.0, 0.0, 0.3 };

		render_sun_light(projection, model_view, moon_direction, moon_color);
	}

	render_forward_pass();
	{
		// render transparent stuff
	}

	render_finish();

	/* Render UI */

	glViewport(0, 0, screenw, screenh);

	mat_ortho(projection, 0, screenw, screenh, 0, -1, 1);
	mat_identity(model_view);

	render_blit(projection, screenw, screenh);
	//render_debug_buffers(projection);

	if (showconsole)
		console_draw(projection, droid_sans_mono, 15);

	glutSwapBuffers();

	i = glGetError();
	if (i)
		fprintf(stderr, "opengl error %d\n", i);
}

int main(int argc, char **argv)
{
	int i;

	glutInit(&argc, argv);
	glutInitWindowPosition(50, 50+24);
	glutInitWindowSize(screenw, screenh);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("Mio");

	gl3wInit();

	fprintf(stderr, "OpenGL %s; ", glGetString(GL_VERSION));
	fprintf(stderr, "GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);

	render_setup(screenw, screenh);

	register_directory("data/");
	register_archive("data/shapes.zip");
	register_archive("data/textures.zip");

	droid_sans = load_font("fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);

	console_init();

	model = load_model("tr_mo_kami_caster.iqm");

	glutMainLoop();
	return 0;
}
