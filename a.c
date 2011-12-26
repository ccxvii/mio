#include "mio.h"

static int screenw = 800, screenh = 600;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;

static int showconsole = 0;
static int showskeleton = 0;
static int showplane = 1;
static int showwire = 0;
static int showalpha = 0;
static int showicon = 0;
static int showbackface = 0;
static int retarget = 0;

static int lasttime = 0;
static int showanim = 0;
static int animspeed = 30;
static float animtick = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static struct model *model;
static struct animation *animation;

mat4 skin_matrix[MAXBONE];
mat4 pose_matrix[MAXBONE];
mat4 anim_matrix[MAXBONE];
mat4 abs_anim_matrix[MAXBONE];
mat4 abs_pose_matrix[MAXBONE];
struct pose posebuf[MAXBONE];
struct pose animbuf[MAXBONE];

static float cam_dist = 5;
static float cam_yaw = 0;
static float cam_pitch = -20;

struct model *load_model(char *filename)
{
	if (strstr(filename, ".iqm"))
		return load_iqm_model(filename);
	if (strstr(filename, ".iqe"))
		return load_iqe_model(filename);
	if (strstr(filename, ".obj"))
		return load_obj_model(filename);
	fprintf(stderr, "unknown model format: '%s'\n", filename);
	return NULL;
}

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
		case 'i': showicon = MAX(0, showicon+1); break;
		case 'I': showicon = MAX(0, showicon-1); break;
		case 'f': togglefullscreen(); break;
		case ' ': showanim = !showanim; break;
		case '0': animtick = 0; animspeed = 30; break;
		case '.': animtick = floor(animtick) + 1; break;
		case ',': animtick = floor(animtick) - 1; break;
		case '[': animspeed = MAX(5, animspeed - 5); break;
		case ']': animspeed = MIN(60, animspeed + 5); break;
		case 'k': showskeleton = !showskeleton; break;
		case 'p': showplane = !showplane; break;
		case 'w': showwire = !showwire; break;
		case 'b': showbackface = !showbackface; break;
		case 'a': showalpha = !showalpha; break;
		case 'r': retarget++; break;
		case 'R': retarget--; break;
	}

	if (showanim)
		lasttime = glutGet(GLUT_ELAPSED_TIME);

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

	int thistime, timediff;

	thistime = glutGet(GLUT_ELAPSED_TIME);
	timediff = thistime - lasttime;
	lasttime = thistime;

	if (animation) {
		if (showanim) {
			animtick = animtick + (timediff / 1000.0f) * animspeed;
			glutPostRedisplay();
		}
		while (animtick < 0) animtick += animation->frame_count;
		while (animtick >= animation->frame_count) animtick -= animation->frame_count;


		extract_pose(animbuf, animation, (int)animtick);
		calc_pose_matrix(anim_matrix, animbuf, animation->bone_count);
		calc_abs_pose_matrix(abs_anim_matrix, anim_matrix, animation->parent, animation->bone_count);

		retarget = CLAMP(retarget, 0, 4);
		switch (retarget) {
		case 0:
			// copy the animation poses directly
			memcpy(posebuf, model->bind_pose, sizeof posebuf);
			apply_animation(posebuf, model->bone_name, model->bone_count,
					animbuf, animation->bone_name, animation->bone_count);
			calc_pose_matrix(pose_matrix, posebuf, model->bone_count);
			calc_abs_pose_matrix(abs_pose_matrix, pose_matrix, model->parent, model->bone_count);
			break;

		case 1:
			// copy the animation rotation keys directly
			retarget_skeleton_pose_rotate(posebuf,
					model->bind_pose, model->bone_name, model->bone_count,
					animation->bind_pose, animation->bone_name, animation->bone_count,
					animbuf);
			calc_pose_matrix(pose_matrix, posebuf, model->bone_count);
			calc_abs_pose_matrix(abs_pose_matrix, pose_matrix, model->parent, model->bone_count);
			break;

		case 2:
			// retarget using poses (quaternion bind pose diff based)
			retarget_skeleton_pose(posebuf,
					model->bind_pose, model->bone_name, model->bone_count,
					animation->bind_pose, animation->bone_name, animation->bone_count,
					animbuf);
			calc_pose_matrix(pose_matrix, posebuf, model->bone_count);
			calc_abs_pose_matrix(abs_pose_matrix, pose_matrix, model->parent, model->bone_count);
			break;

		case 3:
			// retarget using matrix, in local space (bind pose diff based)
			retarget_skeleton(pose_matrix,
					model->bind_matrix, model->bone_name, model->bone_count,
					animation->bind_matrix, animation->bone_name, animation->bone_count,
					anim_matrix);
			calc_abs_pose_matrix(abs_pose_matrix, pose_matrix, model->parent, model->bone_count);
			break;

		case 4:
			// retarget using matrix, in world space (bind pose diff based)
			retarget_skeleton_world(abs_pose_matrix, model->bind_matrix, model->parent,
					model->abs_bind_matrix, model->bone_name, model->bone_count,
					animation->abs_bind_matrix, animation->bone_name, animation->bone_count,
					abs_anim_matrix);
			break;
		}
	}

	glViewport(0, 0, screenw, screenh);

	glClearColor(0.05, 0.05, 0.05, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)screenw / screenh, 0.1, 1000);
	mat_identity(model_view);
	mat_rotate_x(model_view, -90);

	mat_translate(model_view, 0, cam_dist, -cam_dist / 5);
	mat_rotate_x(model_view, -cam_pitch);
	mat_rotate_z(model_view, -cam_yaw);

	glEnable(GL_DEPTH_TEST);

	if (showplane) {
		draw_begin(projection, model_view);
		draw_set_color(0.1, 0.1, 0.1, 1);
		for (i = -4; i <= 4; i++) {
			draw_line(i, -4, 0, i, 4, 0);
			draw_line(-4, i, 0, 4, i, 0);
		}
		draw_end();
	}

	if (showalpha)
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	if (showbackface)
		glDisable(GL_CULL_FACE);
	if (showwire)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (animation) {
		calc_skin_matrix(skin_matrix, abs_pose_matrix, model->inv_bind_matrix, model->bone_count);
		draw_model_with_pose(model, projection, model_view, skin_matrix);
	} else {
		draw_model(model, projection, model_view);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	glDisable(GL_DEPTH_TEST);

	if (showskeleton && model->bone_count > 0) {
		draw_begin(projection, model_view);
		draw_skeleton_2(model->abs_bind_matrix, model->parent, model->bone_count);
		draw_end();
		if (animation) {
			draw_begin(projection, model_view);
			draw_skeleton_3(abs_pose_matrix, model->parent, model->bone_count);
			draw_end();

			mat_translate(model_view, 2, 0, 0);
			draw_begin(projection, model_view);
			draw_skeleton_2(animation->abs_bind_matrix, animation->parent, animation->bone_count);
			draw_skeleton_3(abs_anim_matrix, animation->parent, animation->bone_count);
			draw_end();
		}
	}

	mat_ortho(projection, 0, screenw, screenh, 0, -1, 1);
	mat_identity(model_view);

	text_begin(projection);
	text_set_font(droid_sans, 20);
	text_set_color(1, 1, 1, 1);
	{
		char buf[256];
		int nelem = 0;
		if (model) {
			for (i = 0; i < model->mesh_count; i++)
				nelem += model->mesh[i].count;
			sprintf(buf, "%d meshes, %d bones, %d triangles.", model->mesh_count, model->bone_count, nelem/3);
			text_show(8, screenh-12, buf);
		}

		if (animation) {
			sprintf(buf, "frame %03d / %03d (%d fps) r:%d", (int)animtick+1, animation->frame_count, animspeed, retarget);
			text_show(8, screenh-12-20, buf);
		}
	}
	text_end();

	if (showicon) {
		int texw, texh;
		glBindTexture(GL_TEXTURE_2D, showicon);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texw);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texh);
		icon_begin(projection);
		icon_set_color(1, 1, 1, 1);
		icon_show(showicon, screenw - texw, screenh - texh, screenw, screenh, 0, 0, 1, 1);
		icon_end();
	}

	if (showconsole)
		console_draw(projection, droid_sans_mono, 15);

	glutSwapBuffers();

	i = glGetError();
	if (i)
		fprintf(stderr, "opengl error %d\n", i);
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowPosition(50, 50+24);
	glutInitWindowSize(screenw, screenh);
	glutInitDisplayMode(GLUT_SRGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
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
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	droid_sans = load_font("data/fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("data/fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);

	if (argc > 1) {
		model = load_model(argv[1]);
		if (argc > 2)
			animation = load_iqm_animation(argv[2]);
	} else {
		model = load_model("data/fo_s2_spiketree.obj");
		animation = NULL;
	}

	console_init();

	glutMainLoop();
	return 0;
}
