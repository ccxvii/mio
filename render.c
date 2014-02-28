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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);

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

/* Fullscreen quad */

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

static const char *blit_vert_src =
	"in vec2 att_position;\n"
	"out vec2 var_texcoord;\n"
	"void main() {\n"
	"	var_texcoord = att_position * 0.5 + 0.5;\n"
	"	gl_Position = vec4(att_position, 0, 1);\n"
	"}\n"
;

static const char *blit_frag_src =
	"uniform sampler2D map_color;\n"
	"in vec2 var_texcoord;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
#ifdef HAVE_SRGB_FRAMEBUFFER
	"	frag_color = texture(map_color, var_texcoord);\n"
#else
	// manually convert to sRGB because final framebuffer is not srgb flagged,
	// due to stupidity in intel drivers on linux...
	"	vec4 color = texture(map_color, var_texcoord);\n"
	"	frag_color = vec4(pow(color.rgb, vec3(1.0/2.2)), color.a);\n"
#endif
	"}\n"
;

void render_blit(mat4 proj, mat4 view, int w, int h)
{
	static int prog = 0;

	// Render fullscreen quad with post-process filter
	if (!prog) {
		prog = compile_shader(blit_vert_src, blit_frag_src);
	}

	glUseProgram(prog);
	glBindTexture(GL_TEXTURE_2D, tex_forward);

#ifdef HAVE_SRGB_FRAMEBUFFER
	draw_fullscreen_quad();
#else
	glDisable(GL_FRAMEBUFFER_SRGB);
	draw_fullscreen_quad();
	glEnable(GL_FRAMEBUFFER_SRGB);
#endif
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
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, PTR(mesh->part[i].first * 2));
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
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, PTR(mesh->part[i].first * 2));
	}
}

/* Point lamp */

static const char *point_frag_src =
	"uniform sampler2D map_color;\n"
	"uniform sampler2D map_normal;\n"
	"uniform sampler2D map_depth;\n"
	"uniform vec2 viewport;\n"
	"uniform mat4 view_from_clip;\n"
	"uniform vec3 lamp_position;\n"
	"uniform vec3 lamp_color;\n"
	"uniform float lamp_distance;\n"
	"uniform bool use_sphere;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / viewport.xy;\n"
	"	float depth = texture(map_depth, texcoord).x;\n"
	"	vec4 pos_clip = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_view = view_from_clip * pos_clip;\n"
	"	vec3 position = pos_view.xyz / pos_view.w;\n"
	"	vec3 normal = texture(map_normal, texcoord).xyz;\n"
	"	vec3 albedo = texture(map_color, texcoord).rgb;\n"
	"	vec3 direction = lamp_position - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = lamp_distance / (lamp_distance + dist2);\n"
	"	if (use_sphere) falloff *= max(lamp_distance - sqrt(dist2), 0.0) / lamp_distance;\n"
	"	vec3 L = normalize(direction);\n"
	"	float diffuse = max(dot(normal, L), 0.0);\n"
	"	frag_color = vec4(albedo * lamp_color * diffuse * falloff, 0);\n"
	"}\n"
;

void render_point_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world, mat4 lamp_transform)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_view_from_clip;
	static int uni_lamp_position;
	static int uni_lamp_color;
	static int uni_lamp_distance;
	static int uni_use_sphere;

	mat4 view_from_clip;
	vec2 viewport;
	vec3 lamp_position;
	vec3 lamp_color;

	if (!prog) {
		prog = compile_shader(quad_vert_src, point_frag_src);
		uni_viewport = glGetUniformLocation(prog, "viewport");
		uni_view_from_clip = glGetUniformLocation(prog, "view_from_clip");
		uni_lamp_position = glGetUniformLocation(prog, "lamp_position");
		uni_lamp_color = glGetUniformLocation(prog, "lamp_color");
		uni_lamp_distance = glGetUniformLocation(prog, "lamp_distance");
		uni_use_sphere = glGetUniformLocation(prog, "use_sphere");
	}

	mat_invert(view_from_clip, clip_from_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(lamp_position, view_from_world, lamp_transform + 12);
	vec_scale(lamp_color, lamp->color, lamp->energy);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_view_from_clip, 1, 0, view_from_clip);
	glUniform3fv(uni_lamp_position, 1, lamp_position);
	glUniform3fv(uni_lamp_color, 1, lamp_color);
	glUniform1f(uni_lamp_distance, lamp->distance);
	glUniform1i(uni_use_sphere, lamp->use_sphere);

	draw_fullscreen_quad();
}

/* Spot lamp */

static const char *spot_frag_src =
	"uniform sampler2D map_color;\n"
	"uniform sampler2D map_normal;\n"
	"uniform sampler2D map_depth;\n"
	"uniform vec2 viewport;\n"
	"uniform mat4 view_from_clip;\n"
	"uniform vec3 lamp_position;\n"
	"uniform vec3 lamp_direction;\n"
	"uniform vec3 lamp_color;\n"
	"uniform float lamp_distance;\n"
	"uniform float spot_size;\n"
	"uniform float spot_blend;\n"
	"uniform bool use_sphere;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / viewport.xy;\n"
	"	float depth = texture(map_depth, texcoord).x;\n"
	"	vec4 pos_clip = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_view = view_from_clip * pos_clip;\n"
	"	vec3 position = pos_view.xyz / pos_view.w;\n"
	"	vec3 normal = texture(map_normal, texcoord).xyz;\n"
	"	vec3 albedo = texture(map_color, texcoord).rgb;\n"
	"	vec3 direction = lamp_position - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = lamp_distance / (lamp_distance + dist2);\n"
	"	if (use_sphere) falloff *= max(lamp_distance - sqrt(dist2), 0.0) / lamp_distance;\n"
	"	vec3 L = normalize(direction);\n"
	"	float diffuse = max(dot(normal, L), 0.0) * falloff;\n"
	"	float spot_dot = dot(lamp_direction, L);\n"
	"	float shadow = 1.0;\n"
	"	if (spot_dot <= spot_size) shadow = 0.0;\n"
	"	else if (spot_blend != 0.0) shadow = smoothstep(0.0, 1.0, (spot_dot - spot_size) / spot_blend);\n"
	"	frag_color = vec4(albedo * diffuse * lamp_color * shadow, 0);\n"
	"}\n"
;

void render_spot_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world, mat4 lamp_transform)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_view_from_clip;
	static int uni_lamp_position;
	static int uni_lamp_direction;
	static int uni_lamp_color;
	static int uni_lamp_distance;
	static int uni_spot_size;
	static int uni_spot_blend;
	static int uni_use_sphere;

	static const vec3 lamp_direction_init = { 0, 0, 1 };

	mat4 view_from_clip;
	vec2 viewport;
	vec3 lamp_position;
	vec3 lamp_direction_world;
	vec3 lamp_direction_view;
	vec3 lamp_direction;
	vec3 lamp_color;
	float spot_size;
	float spot_blend;

	if (!prog) {
		prog = compile_shader(quad_vert_src, spot_frag_src);
		uni_viewport = glGetUniformLocation(prog, "viewport");
		uni_view_from_clip = glGetUniformLocation(prog, "view_from_clip");
		uni_lamp_position = glGetUniformLocation(prog, "lamp_position");
		uni_lamp_direction = glGetUniformLocation(prog, "lamp_direction");
		uni_lamp_color = glGetUniformLocation(prog, "lamp_color");
		uni_lamp_distance = glGetUniformLocation(prog, "lamp_distance");
		uni_spot_size = glGetUniformLocation(prog, "spot_size");
		uni_spot_blend = glGetUniformLocation(prog, "spot_blend");
		uni_use_sphere = glGetUniformLocation(prog, "use_sphere");
	}

	mat_invert(view_from_clip, clip_from_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(lamp_position, view_from_world, lamp_transform + 12);

	mat_vec_mul_n(lamp_direction_world, lamp_transform, lamp_direction_init);
	mat_vec_mul_n(lamp_direction_view, view_from_world, lamp_direction_world);
	vec_normalize(lamp_direction, lamp_direction_view);

	vec_scale(lamp_color, lamp->color, lamp->energy);

	spot_size = cos(M_PI * lamp->spot_angle / 360.0);
	spot_blend = (1.0 - spot_size) * lamp->spot_blend;

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_view_from_clip, 1, 0, view_from_clip);
	glUniform3fv(uni_lamp_position, 1, lamp_position);
	glUniform3fv(uni_lamp_direction, 1, lamp_direction);
	glUniform3fv(uni_lamp_color, 1, lamp_color);
	glUniform1f(uni_lamp_distance, lamp->distance);
	glUniform1f(uni_spot_size, spot_size);
	glUniform1f(uni_spot_blend, spot_blend);
	glUniform1i(uni_use_sphere, lamp->use_sphere);

	draw_fullscreen_quad();
}

static const char *sun_frag_src =
	"uniform sampler2D map_color;\n"
	"uniform sampler2D map_normal;\n"
	"uniform vec2 viewport;\n"
	"uniform vec3 lamp_direction;\n"
	"uniform vec3 lamp_color;\n"
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / viewport.xy;\n"
	"	vec3 normal = texture(map_normal, texcoord).xyz;\n"
	"	vec3 albedo = texture(map_color, texcoord).rgb;\n"
	"	float diffuse = max(dot(normal, lamp_direction), 0.0);\n"
	"	frag_color = vec4(albedo * diffuse * lamp_color, 0);\n"
	"}\n"
;

void render_sun_lamp(struct lamp *lamp, mat4 clip_from_view, mat4 view_from_world, mat4 lamp_transform)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_lamp_direction;
	static int uni_lamp_color;

	static const vec3 lamp_direction_init = { 0, 0, 1 };

	vec2 viewport;
	vec3 lamp_direction_world;
	vec3 lamp_direction_view;
	vec3 lamp_direction;
	vec3 lamp_color;

	if (!prog) {
		prog = compile_shader(quad_vert_src, sun_frag_src);
		uni_viewport = glGetUniformLocation(prog, "viewport");
		uni_lamp_direction = glGetUniformLocation(prog, "lamp_direction");
		uni_lamp_color = glGetUniformLocation(prog, "lamp_color");
	}

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul_n(lamp_direction_world, lamp_transform, lamp_direction_init);
	mat_vec_mul_n(lamp_direction_view, view_from_world, lamp_direction_world);
	vec_normalize(lamp_direction, lamp_direction_view);

	vec_scale(lamp_color, lamp->color, lamp->energy);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniform3fv(uni_lamp_direction, 1, lamp_direction);
	glUniform3fv(uni_lamp_color, 1, lamp_color);

	draw_fullscreen_quad();
}

static const char *sky_vert_src =
	"in vec4 att_position;\n"
	"void main() {\n"
	"	gl_Position = vec4(att_position.xy, 1, 1);\n"
	"}\n"
;

static const char *sky_frag_src =
	"out vec4 frag_color;\n"
	"void main() {\n"
	"	frag_color = vec4(0.05, 0.05, 0.05, 1);\n"
	"}\n"
;

void render_sky(void)
{
	static int prog = 0;

	if (!prog) {
		prog = compile_shader(sky_vert_src, sky_frag_src);
	}

	glUseProgram(prog);
	draw_fullscreen_quad();
}
