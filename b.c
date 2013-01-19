#include "mio.h"

static int screenw = 1440, screenh = 900;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;

static int showconsole = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static struct mesh *mesh;

static float cam_dist = 5;
static float cam_yaw = 0;
static float cam_pitch = -20;

static float X = 0;

static int spot_shadow = 0;
static int sun_shadow = 0;
static int moon_shadow = 0;
static int fill_shadow = 0;

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
	if ((mod & GLUT_ACTIVE_ALT) && key == '\r')
		togglefullscreen();
	else if (key ==	'`')
		showconsole = !showconsole;
	else if (showconsole)
		console_update(key, mod);
	else switch (key) {
		case 27: case 'q': exit(0); break;
		case 'f': togglefullscreen(); break;
		case 'x': X -= 0.1; break;
		case 'X': X += 0.1; break;
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

	if (!spot_shadow) spot_shadow = alloc_shadow_map();
	if (!sun_shadow) sun_shadow = alloc_shadow_map();
	if (!moon_shadow) moon_shadow = alloc_shadow_map();
	if (!fill_shadow) fill_shadow = alloc_shadow_map();

	/* Render world */

	glViewport(0, 0, screenw, screenh);

	glClearColor(0.05, 0.05, 0.05, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)screenw / screenh, 0.5, 100);
	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_translate(model_view, 0, cam_dist, -cam_dist / 5);
	mat_rotate_x(model_view, -cam_pitch);
	mat_rotate_z(model_view, -cam_yaw);

	vec3 spot_color = { SRGB(0.8, 0.8, 0.8) };
	vec3 spot_position = { 0, 0, 10 };
	vec3 spot_direction = { 0, 0, -1 };
	float spot_distance = 50.0;
	float spot_angle = 45.0;

	vec3 sky_position = { 0, 0, 5 };

	vec3 sun_color = { SRGB(0.8, 0.8, 0.8) };
	vec3 sun_direction = { 0.892, 0.899, -0.300 };

	vec3 moon_color = { SRGB(0.498, 0.500, 0.600) };
	vec3 moon_direction = { -0.588, 0.248, -0.460 };

	vec3 fill_color = { SRGB(0.798, 0.838, 1.0) };
	vec3 fill_direction = { -0.216, -0.216, 0.392 };

	spot_position[0] = X;

	render_spot_shadow(spot_shadow, spot_position, spot_direction, spot_angle, spot_distance);
	{
		render_mesh_shadow(mesh);
	}

#define SUN_W 10
#define SUN_D 10

	render_sun_shadow(sun_shadow, sky_position, sun_direction, SUN_W, SUN_D);
	render_mesh_shadow(mesh);

	render_sun_shadow(moon_shadow, sky_position, moon_direction, SUN_W, SUN_D);
	render_mesh_shadow(mesh);

	render_sun_shadow(fill_shadow, sky_position, fill_direction, SUN_W, SUN_D);
	render_mesh_shadow(mesh);

	render_geometry_pass();
	{
		render_mesh(mesh, projection, model_view);
	}

	render_light_pass();
	{
		render_sun_light(sun_shadow, projection, model_view, sky_position, sun_direction, SUN_W, SUN_D, sun_color);
		render_sun_light(moon_shadow, projection, model_view, sky_position, moon_direction, SUN_W, SUN_D, moon_color);
		render_sun_light(fill_shadow, projection, model_view, sky_position, fill_direction, SUN_W, SUN_D, fill_color);

	//	static vec3 torch_position = { 1, 0, 1 };
	//	static vec3 torch_color = { 1.0, 1.0, 1.0 };
	//	static float torch_distance = 25.0;
	//	torch_position[1] = X;
	//	render_point_light(projection, model_view, torch_position, torch_color, torch_distance);

		render_spot_light(spot_shadow, projection, model_view, spot_position, spot_direction, spot_angle, spot_color, spot_distance);
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

	//render_blit(projection, screenw, screenh);
	render_debug_buffers(projection);

	icon_begin(projection);
	icon_show(spot_shadow, 0, 0, 128, 128, 0, 0, 1, 1);
	icon_show(sun_shadow, 128, 0, 256, 128, 0, 0, 1, 1);
	icon_end();

	if (showconsole)
		console_draw(projection, droid_sans_mono, 15);

	glutSwapBuffers();

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

	fprintf(stderr, "OpenGL %s; ", glGetString(GL_VERSION));
	fprintf(stderr, "GLSL %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
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

	if (argc > 1)
		mesh = load_mesh(argv[1]);
	else
		mesh = load_mesh("untitled.iqe");

	glutMainLoop();
	return 0;
}
