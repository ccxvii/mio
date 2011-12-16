#include "mio.h"

#include <time.h>

static int screenw = 800, screenh = 600;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static int icon;

static struct model *model;
static struct animation *animation;

mat4 matbuf[MAXBONE];
struct pose posebuf[MAXBONE];

static float cam_dist = 10;
static float cam_yaw = -5;
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
	if (mousemiddle) {
		cam_dist += dy * 0.01 * cam_dist;
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
	else if ((mod & GLUT_ACTIVE_ALT) && key == GLUT_KEY_F4)
		exit(0);
	else if (key == 27)
		exit(0);
	else
		console_update(key, mod);
	glutPostRedisplay();
}

static void special(int key, int x, int y)
{
	keyboard(key, x, y);
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

	glViewport(0, 0, screenw, screenh);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)screenw / screenh, 0.05, 5000);
	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_translate(model_view, 0, cam_dist, -cam_dist / 3);
	mat_rotate_x(model_view, -cam_pitch);
	mat_rotate_z(model_view, -cam_yaw);

	glEnable(GL_DEPTH_TEST);

	draw_begin(projection, model_view);
	draw_set_color(0.5, 0.5, 0.5, 1);
	for (i = -4; i <= 4; i++) {
		draw_line(i, -4, 0, i, 4, 0);
		draw_line(-4, i, 0, 4, i, 0);
	}
	draw_end();

	if (animation) {
		static int f = 0;
		memcpy(posebuf, model->bind_pose, model->bone_count * sizeof(struct pose));
		extract_pose(posebuf, animation, f++ % 100);
//		apply_pose2(matbuf, model->bone, model->bind_pose, model->bone_count);
		apply_pose2(matbuf, model->bone, posebuf, model->bone_count);
		draw_model_with_pose(model, projection, model_view, matbuf);
	} else {
		draw_model(model, projection, model_view);
	}

	glDisable(GL_DEPTH_TEST);

	if (animation) {
		draw_begin(projection, model_view);

		apply_pose(matbuf, model->bone, model->bind_pose, model->bone_count);
		draw_set_color(1, 0, 0, 0.4);
		draw_skeleton(model->bone, matbuf, model->bone_count);

		apply_pose(matbuf, model->bone, posebuf, model->bone_count);
		draw_set_color(0, 1, 0, 0.4);
		draw_skeleton(model->bone, matbuf, model->bone_count);

		draw_end();
	}

	mat_ortho(projection, 0, screenw, screenh, 0, -1, 1);
	mat_identity(model_view);

	text_begin(projection);
	text_set_font(droid_sans, 20);
	{
		char buf[80];
		time_t now = time(NULL);
		strcpy(buf, ctime(&now));
		buf[strlen(buf)-1] = 0; // zap newline.
		text_set_color(1, 1, 1, 1);
		text_show(8, screenh-12, buf);
	}
	text_end();

	icon_begin(projection);
	icon_set_color(1, 1, 1, 1);
	icon_show(icon, screenw - 128, screenh - 128,  screenw, screenh, 0, 0, 1, 1);
	icon_end();

	console_draw(projection, droid_sans_mono, 15);

	glutSwapBuffers();
}

int main(int argc, char **argv)
{
	int i;

	glutInit(&argc, argv);
	glutInitWindowPosition(50, 50+24);
	glutInitWindowSize(screenw, screenh);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("Mio");

	gl3wInit();
	fprintf(stderr, "OpenGL %s; ", glGetString(GL_VERSION));
	fprintf(stderr, "GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &i);
	fprintf(stderr, "GL_MAX_VERTEX_UNIFORM_COMPONENTS = %d\n", i);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &i);
	fprintf(stderr, "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS = %d\n", i);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);

	glutIdleFunc(glutPostRedisplay);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);

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

	glutMainLoop();
	return 0;
}
