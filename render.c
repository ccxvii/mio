/* Deferred shading */

#include "mio.h"

/* Deferred shader */

static int fbo_w = 0, fbo_h = 0;

static unsigned int fbo_geometry = 0;
static unsigned int tex_depth = 0;
static unsigned int tex_normal = 0;
static unsigned int tex_albedo = 0;

static unsigned int fbo_forward = 0;
static unsigned int tex_forward = 0;

static void render_init(void)
{
	if (fbo_geometry)
		return;

	glGenTextures(1, &tex_depth);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &tex_normal);
	glBindTexture(GL_TEXTURE_2D, tex_normal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &tex_albedo);
	glBindTexture(GL_TEXTURE_2D, tex_albedo);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &tex_forward);
	glBindTexture(GL_TEXTURE_2D, tex_forward);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* For the geometry pass */

	static const GLenum geometry_list[] = { GL_COLOR_ATTACHMENT0 + FRAG_NORMAL, GL_COLOR_ATTACHMENT0 + FRAG_ALBEDO};

	glGenFramebuffers(1, &fbo_geometry);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_geometry);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + FRAG_NORMAL, GL_TEXTURE_2D, tex_normal, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + FRAG_ALBEDO, GL_TEXTURE_2D, tex_albedo, 0);

	glReadBuffer(GL_NONE);
	glDrawBuffers(nelem(geometry_list), geometry_list);

	/* For the light accumulation and forward passes */

	static const GLenum forward_list[] = { GL_COLOR_ATTACHMENT0 };

	glGenFramebuffers(1, &fbo_forward);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_forward);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_forward, 0);

	glReadBuffer(GL_NONE);
	glDrawBuffers(nelem(forward_list), forward_list);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_reshape(int w, int h)
{
	render_init();

	if (fbo_w == w && fbo_h == h)
		return;

	fbo_w = w;
	fbo_h = h;

	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, tex_normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, tex_albedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, tex_forward);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_geometry);
	gl_assert_framebuffer(GL_FRAMEBUFFER, "geometry");

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_forward);
	gl_assert_framebuffer(GL_FRAMEBUFFER, "deferred");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* Depth testing, no blending, to geometry buffer */
void render_geometry_pass(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_geometry);
	glViewport(0, 0, fbo_w, fbo_h);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
}

/* No depth testing, additive blending, to light buffer */
void render_light_pass(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_forward);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glActiveTexture(MAP_DEPTH);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glActiveTexture(MAP_NORMAL);
	glBindTexture(GL_TEXTURE_2D, tex_normal);
	glActiveTexture(MAP_COLOR);
	glBindTexture(GL_TEXTURE_2D, tex_albedo);
}

/* Depth testing, no depth writing, additive blending, to light buffer */
void render_forward_pass(void)
{
	glActiveTexture(MAP_SHADOW);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(MAP_DEPTH);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(MAP_NORMAL);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(MAP_COLOR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
}

/* Reset state and framebuffer to default */
void render_finish(void)
{
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_blit(mat4 proj, mat4 view, int w, int h)
{
	// Render fullscreen quad with post-process filter
	icon_begin(proj, view);
	icon_show(tex_forward, 0, 0, w, h, 0, 1, 1, 0);
	icon_end();

	// Blit framebuffers (maybe faster than quad?)
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_forward);
//	glBlitFramebuffer(0, 0, fbo_w, fbo_h, 0, 0, fbo_w, fbo_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void render_debug_buffers(mat4 proj, mat4 view)
{
	int w = fbo_w / 2;
	int h = fbo_h / 2;

	icon_begin(proj, view);
	glDisable(GL_BLEND);
	icon_show(tex_albedo, 0, 0, w, h, 0, 1, 1, 0);
	icon_show(tex_normal, w, 0, w+w, h, 0, 1, 1, 0);
	icon_show(tex_depth, 0, h, w, h+h, 0, 1, 1, 0);
	icon_show(tex_forward, w, h, w+w, h+h, 0, 1, 1, 0);
	icon_end();
}

/* draw model */

static const char *static_mesh_vert_src =
	"uniform mat4 clip_from_view;\n"
	"uniform mat4 view_from_model;\n"
	"in vec4 att_position;\n"
	"in vec3 att_normal;\n"
	"in vec2 att_texcoord;\n"
	"out vec3 var_normal;\n"
	"out vec2 var_texcoord;\n"
	"void main() {\n"
	"	gl_Position = clip_from_view * view_from_model * att_position;\n"
	"	vec4 normal = view_from_model * vec4(att_normal, 0.0);\n"
	"	var_normal = normalize(normal.xyz);\n"
	"	var_texcoord = att_texcoord;\n"
	"}\n"
;

#define STR1(s) #s
#define STR2(s) STR1(s)

static const char *skinned_mesh_vert_src =
	"uniform mat4 clip_from_view;\n"
	"uniform mat4 view_from_model;\n"
	"uniform mat4 model_from_bind_pose[" STR2(MAXBONE) "];\n"
	"in vec4 att_position;\n"
	"in vec3 att_normal;\n"
	"in vec2 att_texcoord;\n"
	"in vec4 att_blend_index;\n"
	"in vec4 att_blend_weight;\n"
	"out vec3 var_normal;\n"
	"out vec2 var_texcoord;\n"
	"void main() {\n"
	"	vec4 position = vec4(0);\n"
	"	vec4 normal = vec4(0);\n"
	"	vec4 index = att_blend_index;\n"
	"	vec4 weight = att_blend_weight;\n"
	"	for (int i = 0; i < 4; i++) {\n"
	"		mat4 m = model_from_bind_pose[int(index.x)];\n"
	"		position += m * att_position * weight.x;\n"
	"		normal += m * vec4(att_normal, 0) * weight.x;\n"
	"		index = index.yzwx;\n"
	"		weight = weight.yzwx;\n"
	"	}\n"
	"	position = view_from_model * position;\n"
	"	normal = view_from_model * normal;\n"
	"	gl_Position = clip_from_view * position;\n"
	"	var_normal = normal.xyz;\n"
	"	var_texcoord = att_texcoord;\n"
	"}\n"
;

static const char *mesh_frag_src =
	"uniform sampler2D map_color;\n"
	"in vec3 var_normal;\n"
	"in vec2 var_texcoord;\n"
	"out vec4 frag_normal;\n"
	"out vec4 frag_albedo;\n"
	"void main() {\n"
	"	vec4 albedo = texture(map_color, var_texcoord);\n"
	"	vec3 normal = normalize(var_normal);\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	frag_normal = vec4(normal.xyz, 0);\n"
	"	frag_albedo = vec4(albedo.rgb, 1);\n"
	"}\n"
;

void render_static_mesh(struct mesh *mesh, mat4 clip_from_view, mat4 view_from_model)
{
	static int prog = 0;
	static int uni_clip_from_view;
	static int uni_view_from_model;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(static_mesh_vert_src, mesh_frag_src);
		uni_clip_from_view = glGetUniformLocation(prog, "clip_from_view");
		uni_view_from_model = glGetUniformLocation(prog, "view_from_model");
	}

	glUseProgram(prog);
	glUniformMatrix4fv(uni_clip_from_view, 1, 0, clip_from_view);
	glUniformMatrix4fv(uni_view_from_model, 1, 0, view_from_model);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}
}

void render_skinned_mesh(struct mesh *mesh, mat4 clip_from_view, mat4 view_from_model, mat4 *model_from_bind_pose)
{
	static int prog = 0;
	static int uni_clip_from_view;
	static int uni_view_from_model;
	static int uni_model_from_bind_pose;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(skinned_mesh_vert_src, mesh_frag_src);
		uni_clip_from_view = glGetUniformLocation(prog, "clip_from_view");
		uni_view_from_model = glGetUniformLocation(prog, "view_from_model");
		uni_model_from_bind_pose = glGetUniformLocation(prog, "model_from_bind_pose");
	}

	glUseProgram(prog);
	glUniformMatrix4fv(uni_clip_from_view, 1, 0, clip_from_view);
	glUniformMatrix4fv(uni_view_from_model, 1, 0, view_from_model);
	glUniformMatrix4fv(uni_model_from_bind_pose, mesh->skel->count, 0, model_from_bind_pose[0]);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}
}

/* draw lights */

static const char *quad_vert_src =
	"in vec4 att_position;\n"
	"void main() {\n"
	"	gl_Position = att_position;\n"
	"}\n"
;

static void draw_fullscreen_quad(void)
{
	static float quad_vertex[] = { -1, 1, -1, -1, 1, 1, 1, -1 };
	static unsigned int quad_vao = 0;
	static unsigned int quad_vbo = 0;

	if (!quad_vao) {
		glGenVertexArrays(1, &quad_vao);
		glGenBuffers(1, &quad_vbo);

		glBindVertexArray(quad_vao);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), quad_vertex, GL_STATIC_DRAW);

		glEnableVertexAttribArray(ATT_POSITION);
		glVertexAttribPointer(ATT_POSITION, 2, GL_FLOAT, 0, 0, 0);
	}

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/* Point light */

static const char *point_frag_src =
	"uniform sampler2D map_color;\n"
	"uniform sampler2D map_normal;\n"
	"uniform sampler2D map_depth;\n"
	"uniform vec2 viewport;\n"
	"uniform mat4 view_from_clip;\n"
	"uniform vec3 light_position;\n"
	"uniform vec3 light_color;\n"
	"uniform float light_distance;\n"
	"out vec4 frag_color;\n"

	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / viewport.xy;\n"
	"	float depth = texture(map_depth, texcoord).x;\n"
	"	vec4 pos_clip = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_view = view_from_clip * pos_clip;\n"
	"	vec3 position = pos_view.xyz / pos_view.w;\n"
	"	vec3 normal = texture(map_normal, texcoord).xyz;\n"
	"	vec3 albedo = texture(map_color, texcoord).rgb;\n"

	"	vec3 direction = light_position - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = light_distance / (light_distance + dist2);\n"
	"	falloff = falloff * max(light_distance - sqrt(dist2), 0.0) / light_distance;\n"
	"	vec3 L = normalize(direction);\n"
	"	float diffuse = max(dot(normal, L), 0.0) * falloff;\n"

	"	frag_color = vec4(albedo * diffuse * light_color, 0);\n"
	"}\n"
;

void render_point_light(struct light *light, mat4 clip_from_view, mat4 view_from_world)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_view_from_clip;
	static int uni_light_position;
	static int uni_light_color;
	static int uni_light_distance;

	mat4 view_from_clip;
	vec2 viewport;
	vec3 light_position;
	vec3 light_color;

	if (!prog) {
		prog = compile_shader(quad_vert_src, point_frag_src);
		uni_viewport = glGetUniformLocation(prog, "viewport");
		uni_view_from_clip = glGetUniformLocation(prog, "view_from_clip");
		uni_light_position = glGetUniformLocation(prog, "light_position");
		uni_light_color = glGetUniformLocation(prog, "light_color");
		uni_light_distance = glGetUniformLocation(prog, "light_distance");
	}

	mat_invert(view_from_clip, clip_from_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(light_position, view_from_world, light->transform + 12);
	vec_scale(light_color, light->color, light->energy);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_view_from_clip, 1, 0, view_from_clip);
	glUniform3fv(uni_light_position, 1, light_position);
	glUniform3fv(uni_light_color, 1, light_color);
	glUniform1f(uni_light_distance, light->distance);

	draw_fullscreen_quad();
}

/* Spot light */

static const char *spot_frag_src =
	"uniform sampler2D map_color;\n"
	"uniform sampler2D map_normal;\n"
	"uniform sampler2D map_depth;\n"
	"uniform vec2 viewport;\n"
	"uniform mat4 view_from_clip;\n"
	"uniform vec3 light_position;\n"
	"uniform vec3 light_direction;\n"
	"uniform vec3 light_color;\n"
	"uniform float light_distance;\n"
	"uniform float light_angle;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / viewport.xy;\n"
	"	float depth = texture(map_depth, texcoord).x;\n"
	"	vec4 pos_clip = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_view = view_from_clip * pos_clip;\n"
	"	vec3 position = pos_view.xyz / pos_view.w;\n"
	"	vec3 normal = texture(map_normal, texcoord).xyz;\n"
	"	vec3 albedo = texture(map_color, texcoord).rgb;\n"

	"	vec3 direction = light_position - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = light_distance / (light_distance + dist2);\n"
	"	falloff = falloff * max(light_distance - sqrt(dist2), 0.0) / light_distance;\n"
	"	vec3 L = normalize(direction);\n"
	"	float diffuse = max(dot(normal, L), 0.0) * falloff;\n"

	"	float spot = dot(light_direction, L);\n"
	"	float shadow = 1.0;\n"
	"	if (spot <= light_angle) { shadow = 0.0; }\n"

	"	frag_color = vec4(albedo * diffuse * light_color * shadow, 0);\n"
	"}\n"
;

void render_spot_light(struct light *light, mat4 clip_from_view, mat4 view_from_world)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_view_from_clip;
	static int uni_light_position;
	static int uni_light_direction;
	static int uni_light_angle;
	static int uni_light_color;
	static int uni_light_distance;

	static const vec3 light_direction_init = { 0, 0, 1 };

	mat4 view_from_clip;
	vec2 viewport;
	vec3 light_position;
	vec3 light_direction_world;
	vec3 light_direction_view;
	vec3 light_direction;
	vec3 light_color;

	if (!prog) {
		prog = compile_shader(quad_vert_src, spot_frag_src);
		uni_viewport = glGetUniformLocation(prog, "viewport");
		uni_view_from_clip = glGetUniformLocation(prog, "view_from_clip");
		uni_light_position = glGetUniformLocation(prog, "light_position");
		uni_light_direction = glGetUniformLocation(prog, "light_direction");
		uni_light_angle = glGetUniformLocation(prog, "light_angle");
		uni_light_color = glGetUniformLocation(prog, "light_color");
		uni_light_distance = glGetUniformLocation(prog, "light_distance");
	}

	mat_invert(view_from_clip, clip_from_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(light_position, view_from_world, light->transform + 12);

	mat_vec_mul_n(light_direction_world, light->transform, light_direction_init);
	mat_vec_mul_n(light_direction_view, view_from_world, light_direction_world);
	vec_normalize(light_direction, light_direction_view);

	vec_scale(light_color, light->color, light->energy);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_view_from_clip, 1, 0, view_from_clip);
	glUniform3fv(uni_light_position, 1, light_position);
	glUniform3fv(uni_light_direction, 1, light_direction);
	glUniform3fv(uni_light_color, 1, light_color);
	glUniform1f(uni_light_distance, light->distance);
	glUniform1f(uni_light_angle, cos(light->spot_angle * 0.5 * 3.14157 / 180.0));

	draw_fullscreen_quad();
}

void render_sun_light(struct light *light, mat4 clip_from_view, mat4 view_from_world)
{
}
