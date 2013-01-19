/* Deferred shading */

#include "mio.h"

static unsigned int fbo_shadow = 0;
static unsigned int shadow_size = 512;

static const mat4 bias_matrix = {
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.5f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f
};

static mat4 shadow_projection;
static mat4 shadow_model_view;
static mat4 shadow_matrix;

int alloc_shadow_map(void)
{
	unsigned int tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_size, shadow_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	if (!fbo_shadow)
	{
		glGenFramebuffers(1, &fbo_shadow);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);

		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);

		gl_assert_framebuffer(GL_FRAMEBUFFER, "shadow");
	}

	return tex;
}

static void setup_spot_shadow(vec3 spot_position, vec3 spot_direction, float spot_angle, float distance)
{
	mat_perspective(shadow_projection, spot_angle, 1.0, 0.1, distance);
	vec3 up;
	up[0] = spot_direction[0];
	up[1] = spot_direction[2];
	up[2] = spot_direction[1];
	mat_look(shadow_model_view, spot_position, spot_direction, up);
}

void render_spot_shadow(int shadow_map, vec3 spot_position, vec3 spot_direction, float spot_angle, float distance)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);
	glViewport(0, 0, shadow_size, shadow_size);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	setup_spot_shadow(spot_position, spot_direction, spot_angle, distance);
}

static void setup_sun_shadow(vec3 sun_position, vec3 sun_direction, float width, float depth)
{
	mat_ortho(shadow_projection, -width, width, -width, width, 0, depth);
	vec3 up;
	up[0] = sun_direction[0];
	up[1] = sun_direction[2];
	up[2] = sun_direction[1];
	mat_look(shadow_model_view, sun_position, sun_direction, up);
}

void render_sun_shadow(int shadow_map, vec3 sun_position, vec3 sun_direction, float width, float depth)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);
	glViewport(0, 0, shadow_size, shadow_size);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	setup_sun_shadow(sun_position, sun_direction, width, depth);
}

/* Deferred shader */

static int fbo_w, fbo_h;

static unsigned int fbo_geometry = 0;
static unsigned int tex_depth = 0;
static unsigned int tex_normal = 0;
static unsigned int tex_albedo = 0;

static unsigned int fbo_forward = 0;
static unsigned int tex_forward = 0;

void render_setup(int w, int h)
{
	fbo_w = w;
	fbo_h = h;

	glGenTextures(1, &tex_depth);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glGenTextures(1, &tex_normal);
	glBindTexture(GL_TEXTURE_2D, tex_normal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	glGenTextures(1, &tex_albedo);
	glBindTexture(GL_TEXTURE_2D, tex_albedo);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	glGenTextures(1, &tex_forward);
	glBindTexture(GL_TEXTURE_2D, tex_forward);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, w, h, 0, GL_RGB, GL_FLOAT, NULL);

	/* For the geometry pass */

	static const GLenum geometry_list[] = { GL_COLOR_ATTACHMENT0 + FRAG_NORMAL, GL_COLOR_ATTACHMENT0 + FRAG_ALBEDO};

	glGenFramebuffers(1, &fbo_geometry);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_geometry);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + FRAG_NORMAL, GL_TEXTURE_2D, tex_normal, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + FRAG_ALBEDO, GL_TEXTURE_2D, tex_albedo, 0);

	glReadBuffer(GL_NONE);
	glDrawBuffers(nelem(geometry_list), geometry_list);

	gl_assert_framebuffer(GL_FRAMEBUFFER, "geometry");

	/* For the light accumulation and forward passes */

	static const GLenum forward_list[] = { GL_COLOR_ATTACHMENT0 };

	glGenFramebuffers(1, &fbo_forward);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_forward);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_forward, 0);

	glReadBuffer(GL_NONE);
	glDrawBuffers(nelem(forward_list), forward_list);

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

void render_blit(float projection[16], int w, int h)
{
	// Render fullscreen quad with post-process filter
	icon_begin(projection);
	icon_show(tex_forward, 0, 0, w, h, 0, 1, 1, 0);
	icon_end();

	// Blit framebuffers (maybe faster than quad?)
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_forward);
//	glBlitFramebuffer(0, 0, fbo_w, fbo_h, 0, 0, fbo_w, fbo_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
//	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void render_debug_buffers(float projection[16])
{
	int w = fbo_w / 2;
	int h = fbo_h / 2;

	icon_begin(projection);
	glDisable(GL_BLEND);
	icon_show(tex_albedo, 0, 0, w, h, 0, 1, 1, 0);
	icon_show(tex_normal, w, 0, w+w, h, 0, 1, 1, 0);
	icon_show(tex_depth, 0, h, w, h+h, 0, 1, 1, 0);
	icon_show(tex_forward, w, h, w+w, h+h, 0, 1, 1, 0);
	icon_end();
}

/* draw lights */

static const char *quad_vert_src =
	"in vec4 att_Position;\n"
	"void main() {\n"
	"	gl_Position = att_Position;\n"
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

/* Sun light */

static const char *sun_frag_src =
	"uniform sampler2D map_Color;\n"
	"uniform sampler2D map_Normal;\n"
	"uniform sampler2D map_Depth;\n"
	"uniform sampler2DShadow map_Shadow;\n"
	"uniform vec2 Viewport;\n"
	"uniform mat4 InverseProjection;\n"
	"uniform mat4 ShadowMatrix;\n"
	"uniform vec3 LightDirection;\n"
	"uniform vec3 LightColor;\n"
	"const float Shininess = 64.0;\n"
	"out vec4 frag_Color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / Viewport.xy;\n"
	"	float depth = texture(map_Depth, texcoord).x;\n"
	"	vec4 pos_ndc = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_hom = InverseProjection * pos_ndc;\n"
	"	vec3 position = pos_hom.xyz / pos_hom.w;\n"
	"	vec3 normal = texture(map_Normal, texcoord).xyz;\n"
	"	vec4 albedo = texture(map_Color, texcoord);\n"
	"	vec4 pos_shadow = ShadowMatrix * vec4(position, 1.0);\n"
	"	float shadow = textureProj(map_Shadow, pos_shadow);\n"
	"	vec2 ps = pos_shadow.xy / pos_shadow.w;\n"
	"	if (pos_shadow.w < 0 || ps.x < 0 || ps.x > 1 || ps.y < 0 || ps.y > 1) shadow = 1.0;\n"

	"	float diffuse = max(dot(normal, LightDirection), 0.0);\n"

	"	vec3 V = normalize(-position);\n"
	"	vec3 H = normalize(LightDirection + V);\n"
	"	float specular = pow(max(dot(normal, H), 0.0), Shininess);\n"

	"	frag_Color = vec4((albedo.rgb * diffuse + albedo.a * specular) * shadow * LightColor, 0);\n"
	"}\n"
;

void render_sun_light(int shadow_map, mat4 projection, mat4 model_view, vec3 sun_position, vec3 sun_direction, float width, float depth, vec3 color)
{
	static int sun_prog = 0;
	static int uni_viewport;
	static int uni_inverse_projection;
	static int uni_projection;
	static int uni_model_view;
	static int uni_direction;
	static int uni_color;
	static int uni_shadow_matrix;

	mat4 inverse_projection;
	mat4 inverse_model_view;
	vec2 viewport;
	vec3 direction;

	setup_sun_shadow(sun_position, sun_direction, width, depth);

	if (!sun_prog) {
		sun_prog = compile_shader(quad_vert_src, sun_frag_src);
		uni_viewport = glGetUniformLocation(sun_prog, "Viewport");
		uni_inverse_projection = glGetUniformLocation(sun_prog, "InverseProjection");
		uni_projection = glGetUniformLocation(sun_prog, "Projection");
		uni_model_view = glGetUniformLocation(sun_prog, "ModelView");
		uni_shadow_matrix = glGetUniformLocation(sun_prog, "ShadowMatrix");
		uni_direction = glGetUniformLocation(sun_prog, "LightDirection");
		uni_color = glGetUniformLocation(sun_prog, "LightColor");
	}

	mat_invert(inverse_projection, projection);
	mat_invert(inverse_model_view, model_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul_n(direction, model_view, sun_direction);
	vec_normalize(direction, direction);
	vec_negate(direction, direction);

	// shadow_pos = ShProj * ShMV * inv(MV) * eye_pos
	mat4 tmp1, tmp2;
	mat_mul44(tmp1, shadow_projection, shadow_model_view);
	mat_mul44(tmp2, tmp1, inverse_model_view);
	mat_mul44(shadow_matrix, bias_matrix, tmp2);

	glActiveTexture(MAP_SHADOW);
	glBindTexture(GL_TEXTURE_2D, shadow_map);

	glUseProgram(sun_prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_inverse_projection, 1, 0, inverse_projection);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(uni_shadow_matrix, 1, 0, shadow_matrix);
	glUniform3fv(uni_direction, 1, direction);
	glUniform3fv(uni_color, 1, color);

	draw_fullscreen_quad();
}

/* Point light */

static const char *point_frag_src =
	"uniform sampler2D map_Color;\n"
	"uniform sampler2D map_Normal;\n"
	"uniform sampler2D map_Depth;\n"
	"uniform vec2 Viewport;\n"
	"uniform mat4 InverseProjection;\n"
	"uniform vec3 LightPosition;\n"
	"uniform vec3 LightColor;\n"
	"uniform float LightDistance;\n"
	"const float Shininess = 64.0;\n"
	"out vec4 frag_Color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / Viewport.xy;\n"
	"	float depth = texture(map_Depth, texcoord).x;\n"
	"	vec4 pos_ndc = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_hom = InverseProjection * pos_ndc;\n"
	"	vec3 position = pos_hom.xyz / pos_hom.w;\n"
	"	vec3 normal = texture(map_Normal, texcoord).xyz;\n"
	"	vec4 albedo = texture(map_Color, texcoord);\n"

	"	vec3 direction = LightPosition - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = LightDistance / (LightDistance + dist2);\n"
	"	falloff = falloff * max(LightDistance - sqrt(dist2), 0.0) / LightDistance;\n"
	"	vec3 L = normalize(LightPosition - position);\n"
	"	float diffuse = max(dot(normal, L), 0.0) * falloff;\n"

	"	vec3 V = normalize(-position);\n"
	"	vec3 H = normalize(L + V);\n"
	"	float specular = pow(max(dot(normal, H), 0.0), Shininess) * falloff;\n"

	"	frag_Color = vec4((albedo.rgb * diffuse + albedo.a * specular) * LightColor, 0);\n"
	"}\n"
;

void render_point_light(mat4 projection, mat4 model_view, vec3 point_position, vec3 color, float distance)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_inverse_projection;
	static int uni_projection;
	static int uni_model_view;
	static int uni_position;
	static int uni_color;
	static int uni_distance;

	mat4 inverse_projection;
	vec2 viewport;
	vec3 position;

	if (!prog) {
		prog = compile_shader(quad_vert_src, point_frag_src);
		uni_viewport = glGetUniformLocation(prog, "Viewport");
		uni_inverse_projection = glGetUniformLocation(prog, "InverseProjection");
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
		uni_position = glGetUniformLocation(prog, "LightPosition");
		uni_color = glGetUniformLocation(prog, "LightColor");
		uni_distance = glGetUniformLocation(prog, "LightDistance");
	}

	mat_invert(inverse_projection, projection);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(position, model_view, point_position);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_inverse_projection, 1, 0, inverse_projection);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);
	glUniform3fv(uni_position, 1, position);
	glUniform3fv(uni_color, 1, color);
	glUniform1f(uni_distance, distance);

	draw_fullscreen_quad();
}

/* Spot light */

static const char *spot_frag_src =
	"uniform sampler2D map_Color;\n"
	"uniform sampler2D map_Normal;\n"
	"uniform sampler2D map_Depth;\n"
	"uniform sampler2DShadow map_Shadow;\n"
	"uniform vec2 Viewport;\n"
	"uniform mat4 InverseProjection;\n"
	"uniform mat4 ShadowMatrix;\n"
	"uniform vec3 LightPosition;\n"
	"uniform vec3 LightDirection;\n"
	"uniform float LightAngle;\n"
	"uniform vec3 LightColor;\n"
	"uniform float LightDistance;\n"
	"const float Shininess = 64.0;\n"
	"out vec4 frag_Color;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / Viewport.xy;\n"
	"	float depth = texture(map_Depth, texcoord).x;\n"
	"	vec4 pos_ndc = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_hom = InverseProjection * pos_ndc;\n"
	"	vec3 position = pos_hom.xyz / pos_hom.w;\n"
	"	vec3 normal = texture(map_Normal, texcoord).xyz;\n"
	"	vec4 albedo = texture(map_Color, texcoord);\n"
	"	vec4 pos_shadow = ShadowMatrix * vec4(position, 1.0);\n"
	"	float shadow = textureProj(map_Shadow, pos_shadow);\n"
	"	vec2 ps = pos_shadow.xy / pos_shadow.w;\n"
	"	if (pos_shadow.w < 0 || ps.x < 0 || ps.x > 1 || ps.y < 0 || ps.y > 1) shadow = 0.0;\n"

	"	vec3 direction = LightPosition - position;\n"
	"	float dist2 = dot(direction, direction);\n"
	"	float falloff = LightDistance / (LightDistance + dist2);\n"
	"	falloff = falloff * max(LightDistance - sqrt(dist2), 0.0) / LightDistance;\n"

	"	vec3 L = normalize(direction);\n"
	"	float spot = dot(LightDirection, L);\n"
	"	if (spot <= LightAngle) { shadow = 0.0; }\n"

	"	float diffuse = max(dot(normal, L), 0.0) * falloff;\n"

	"	vec3 V = normalize(-position);\n"
	"	vec3 H = normalize(L + V);\n"
	"	float specular = pow(max(dot(normal, H), 0.0), Shininess) * falloff;\n"

	"	frag_Color = vec4((albedo.rgb * diffuse + albedo.a * specular) * shadow * LightColor, 0);\n"
	"}\n"
;

void render_spot_light(int shadow_map, mat4 projection, mat4 model_view, vec3 spot_position, vec3 spot_direction, float spot_angle, vec3 color, float distance)
{
	static int prog = 0;
	static int uni_viewport;
	static int uni_inverse_projection;
	static int uni_projection;
	static int uni_model_view;
	static int uni_shadow_matrix;
	static int uni_position;
	static int uni_direction;
	static int uni_angle;
	static int uni_color;
	static int uni_distance;

	mat4 inverse_projection;
	mat4 inverse_model_view;
	vec2 viewport;
	vec3 position;
	vec3 direction;

	setup_spot_shadow(spot_position, spot_direction, spot_angle, distance);

	if (!prog) {
		prog = compile_shader(quad_vert_src, spot_frag_src);
		uni_viewport = glGetUniformLocation(prog, "Viewport");
		uni_inverse_projection = glGetUniformLocation(prog, "InverseProjection");
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
		uni_shadow_matrix = glGetUniformLocation(prog, "ShadowMatrix");
		uni_position = glGetUniformLocation(prog, "LightPosition");
		uni_direction = glGetUniformLocation(prog, "LightDirection");
		uni_angle = glGetUniformLocation(prog, "LightAngle");
		uni_color = glGetUniformLocation(prog, "LightColor");
		uni_distance = glGetUniformLocation(prog, "LightDistance");
	}

	mat_invert(inverse_projection, projection);
	mat_invert(inverse_model_view, model_view);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(position, model_view, spot_position);

	mat_vec_mul_n(direction, model_view, spot_direction);
	vec_normalize(direction, direction);
	vec_negate(direction, direction);

	// shadow_pos = ShProj * ShMV * inv(MV) * eye_pos
	mat4 tmp1, tmp2;
	mat_mul44(tmp1, shadow_projection, shadow_model_view);
	mat_mul44(tmp2, tmp1, inverse_model_view);
	mat_mul44(shadow_matrix, bias_matrix, tmp2);

	glActiveTexture(MAP_SHADOW);
	glBindTexture(GL_TEXTURE_2D, shadow_map);

	glUseProgram(prog);
	glUniform2fv(uni_viewport, 1, viewport);
	glUniformMatrix4fv(uni_inverse_projection, 1, 0, inverse_projection);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(uni_shadow_matrix, 1, 0, shadow_matrix);
	glUniform3fv(uni_position, 1, position);
	glUniform3fv(uni_direction, 1, direction);
	glUniform3fv(uni_color, 1, color);
	glUniform1f(uni_distance, distance);
	glUniform1f(uni_angle, cos(spot_angle * 0.5 * 3.14157 / 180.0));

	draw_fullscreen_quad();
}

/* draw model */

static const char *static_vert_src =
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"in vec4 att_Position;\n"
	"in vec3 att_Normal;\n"
	"in vec2 att_TexCoord;\n"
	"out vec3 var_Normal;\n"
	"out vec2 var_TexCoord;\n"
	"void main() {\n"
	"	gl_Position = Projection * ModelView * att_Position;\n"
	"	vec4 normal = ModelView * vec4(att_Normal, 0.0);\n"
	"	var_Normal = normalize(normal.xyz);\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *static_frag_src =
	"uniform sampler2D map_Color;\n"
	"uniform sampler2D map_Gloss;\n"
	"in vec3 var_Normal;\n"
	"in vec2 var_TexCoord;\n"
	"out vec4 frag_Normal;\n"
	"out vec4 frag_Albedo;\n"
	"void main() {\n"
	"	vec4 albedo = texture(map_Color, var_TexCoord);\n"
	"	vec4 gloss = texture(map_Gloss, var_TexCoord);\n"
	"	vec3 normal = normalize(var_Normal);\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	frag_Normal = vec4(normal.xyz, 0);\n"
	"	frag_Albedo = vec4(albedo.rgb, gloss.x);\n"
	"}\n"
;

void render_mesh(struct mesh *mesh, mat4 projection, mat4 model_view)
{
	static int prog = 0;
	static int uni_projection;
	static int uni_model_view;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(static_vert_src, static_frag_src);
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
	}

	glUseProgram(prog);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}
}

static const char *shadow_vert_src =
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"in vec4 att_Position;\n"
	"in vec2 att_TexCoord;\n"
	"out vec2 var_TexCoord;\n"
	"void main() {\n"
	"	gl_Position = Projection * ModelView * att_Position;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *shadow_frag_src =
	"uniform sampler2D map_Color;\n"
	"in vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 albedo = texture(map_Color, var_TexCoord);\n"
	"	if (albedo.a < 0.2) discard;\n"
	"}\n"
;

void render_mesh_shadow(struct mesh *mesh)
{
	static int prog = 0;
	static int uni_projection;
	static int uni_model_view;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(shadow_vert_src, shadow_frag_src);
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
	}

	glPolygonOffset(1.1, 4.0);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glCullFace(GL_FRONT);

	glUseProgram(prog);
	glUniformMatrix4fv(uni_projection, 1, 0, shadow_projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, shadow_model_view);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}

	glCullFace(GL_BACK);
	glDisable(GL_POLYGON_OFFSET_FILL);
}

/* old crap */

static const char *bone_vert_src =
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"uniform mat4 BoneMatrix[80];\n"
	"in vec4 att_Position;\n"
	"in vec3 att_Normal;\n"
	"in vec2 att_TexCoord;\n"
	"in vec4 att_BlendIndex;\n"
	"in vec4 att_BlendWeight;\n"
	"out vec3 var_Normal;\n"
	"out vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 position = vec4(0);\n"
	"	vec4 normal = vec4(0);\n"
	"	vec4 index = att_BlendIndex;\n"
	"	vec4 weight = att_BlendWeight;\n"
	"	for (int i = 0; i < 4; i++) {\n"
	"		mat4 m = BoneMatrix[int(index.x)];\n"
	"		position += m * att_Position * weight.x;\n"
	"		normal += m * vec4(att_Normal, 0) * weight.x;\n"
	"		index = index.yzwx;\n"
	"		weight = weight.yzwx;\n"
	"	}\n"
	"	position = ModelView * position;\n"
	"	normal = ModelView * normal;\n"
	"	gl_Position = Projection * position;\n"
	"	var_Normal = normal.xyz;\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *model_frag_src =
	"uniform sampler2D map_Color;\n"
	"in vec3 var_Position;\n"
	"in vec3 var_Normal;\n"
	"in vec2 var_TexCoord;\n"
	"out vec4 frag_Color;\n"
	"const vec3 LightDirection = vec3(-0.5773, 0.5773, 0.5773);\n"
	"const vec3 LightAmbient = vec3(0.1);\n"
	"const vec3 LightDiffuse = vec3(0.9);\n"
	"void main() {\n"
	"	vec4 albedo = texture(map_Color, var_TexCoord);\n"
	"	vec3 N = normalize(var_Normal);\n"
	"	float diffuse = max(dot(N, LightDirection), 0.0);\n"
	"	vec3 Ka = albedo.rgb * LightAmbient;\n"
	"	vec3 Kd = albedo.rgb * LightDiffuse * diffuse;\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	frag_Color = vec4(Ka + Kd, albedo.a);\n"
	"}\n"
;

void draw_model(struct mesh *mesh, mat4 projection, mat4 model_view)
{
	static int prog = 0;
	static int uni_projection;
	static int uni_model_view;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(static_vert_src, model_frag_src);
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
	}

	glUseProgram(prog);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}
}

void draw_model_with_pose(struct mesh *mesh, mat4 projection, mat4 model_view, mat4 *skin_matrix)
{
	static int prog = 0;
	static int uni_projection;
	static int uni_model_view;
	static int uni_skin_matrix;

	int i;

	if (!mesh)
		return;

	if (!prog) {
		prog = compile_shader(bone_vert_src, model_frag_src);
		uni_projection = glGetUniformLocation(prog, "Projection");
		uni_model_view = glGetUniformLocation(prog, "ModelView");
		uni_skin_matrix = glGetUniformLocation(prog, "BoneMatrix");
	}

	glUseProgram(prog);
	glUniformMatrix4fv(uni_projection, 1, 0, projection);
	glUniformMatrix4fv(uni_model_view, 1, 0, model_view);
	glUniformMatrix4fv(uni_skin_matrix, mesh->skel->count, 0, skin_matrix[0]);

	glBindVertexArray(mesh->vao);

	for (i = 0; i < mesh->count; i++) {
		glActiveTexture(MAP_COLOR);
		glBindTexture(GL_TEXTURE_2D, mesh->part[i].material);
		glDrawElements(GL_TRIANGLES, mesh->part[i].count, GL_UNSIGNED_SHORT, (void*)(mesh->part[i].first * 2));
	}
}
