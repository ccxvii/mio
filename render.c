/* Deferred shading */

#include "mio.h"

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

	static const GLenum geometry_list[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	glGenFramebuffers(1, &fbo_geometry);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_geometry);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_normal, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex_albedo, 0);

	glDrawBuffers(nelem(geometry_list), geometry_list);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "cannot create geometry framebuffer\n");
		exit(1);
	}

	/* For the light accumulation and forward passes */

	static const GLenum forward_list[] = { GL_COLOR_ATTACHMENT0 };

	glGenFramebuffers(1, &fbo_forward);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_forward);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex_depth, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_forward, 0);

	glDrawBuffers(nelem(forward_list), forward_list);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "cannot create forward framebuffer\n");
		exit(1);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/* Depth testing, no blending, to geometry buffer */
void render_geometry_pass(void)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_geometry);
	glViewport(0, 0, fbo_w, fbo_h);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
}

/* No depth testing, additive blending, to light buffer */
void render_light_pass(void)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_forward);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex_normal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex_albedo);
	glActiveTexture(GL_TEXTURE0);
}

/* Depth testing, no depth writing, additive blending, to light buffer */
void render_forward_pass(void)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
}

/* Reset state and framebuffer to default */
void render_finish(void)
{
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
	icon_show(tex_albedo, 0, 0, w, h, 0, 1, 1, 0);
	icon_show(tex_normal, w, 0, w+w, h, 0, 1, 1, 0);
	icon_show(tex_depth, 0, h, w, h+h, 0, 1, 1, 0);
	icon_show(tex_forward, w, h, w+w, h+h, 0, 1, 1, 0);
	icon_end();
}

/* draw lights */

static const char *quad_vert_src =
	"#version 120\n"
	"attribute vec4 att_Position;\n"
	"void main() {\n"
	"	gl_Position = att_Position;\n"
	"}\n"
;

static unsigned int quad_vao = 0;
static unsigned int quad_vbo = 0;
static float quad_vertex[] = {
	-1, 1,
	-1, -1,
	1, 1,
	1, -1,
};

static void draw_fullscreen_quad(void)
{
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
	glBindVertexArray(0);
}

/* Sun light */

static const char *sun_frag_src =
	"#version 120\n"
	"uniform vec2 Viewport;\n"
	"uniform mat4 InverseProjection;\n"
	"uniform sampler2D DepthSampler;\n"
	"uniform sampler2D NormalSampler;\n"
	"uniform sampler2D AlbedoSampler;\n"
	"uniform vec3 LightDirection;\n"
	"uniform vec3 LightColor;\n"
	"const float Shininess = 64.0;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / Viewport.xy;\n"
	"	float depth = texture2D(DepthSampler, texcoord).x;\n"
	"	vec4 pos_ndc = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_hom = InverseProjection * pos_ndc;\n"
	"	vec3 position = pos_hom.xyz / pos_hom.w;\n"
	"	vec3 normal = texture2D(NormalSampler, texcoord).xyz;\n"
	"	vec4 albedo = texture2D(AlbedoSampler, texcoord);\n"

	"	float diffuse = max(dot(normal, LightDirection), 0.0);\n"

	"	vec3 V = normalize(-position);\n"
	"	vec3 H = normalize(LightDirection + V);\n"
	"	float specular = pow(max(dot(normal, H), 0.0), Shininess);\n"

	"	gl_FragColor = vec4((albedo.rgb * diffuse + albedo.a * specular) * LightColor, 1.0);\n"
	"}\n"
;

static int sun_prog = 0;
static int sun_uni_viewport;
static int sun_uni_inverse_projection;
static int sun_uni_projection;
static int sun_uni_model_view;
static int sun_uni_depth_sampler;
static int sun_uni_normal_sampler;
static int sun_uni_albedo_sampler;
static int sun_uni_direction;
static int sun_uni_color;

void render_sun_light(mat4 projection, mat4 model_view, vec3 sun_direction, vec3 color)
{
	mat4 inverse_projection;
	vec2 viewport;
	vec3 direction;

	if (!sun_prog) {
		sun_prog = compile_shader(quad_vert_src, sun_frag_src);
		sun_uni_viewport = glGetUniformLocation(sun_prog, "Viewport");
		sun_uni_inverse_projection = glGetUniformLocation(sun_prog, "InverseProjection");
		sun_uni_projection = glGetUniformLocation(sun_prog, "Projection");
		sun_uni_model_view = glGetUniformLocation(sun_prog, "ModelView");
		sun_uni_depth_sampler = glGetUniformLocation(sun_prog, "DepthSampler");
		sun_uni_normal_sampler = glGetUniformLocation(sun_prog, "NormalSampler");
		sun_uni_albedo_sampler = glGetUniformLocation(sun_prog, "AlbedoSampler");
		sun_uni_direction = glGetUniformLocation(sun_prog, "LightDirection");
		sun_uni_color = glGetUniformLocation(sun_prog, "LightColor");
	}

	mat_invert(inverse_projection, projection);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul_n(direction, model_view, sun_direction);
	vec_normalize(direction);

	glUseProgram(sun_prog);
	glUniform2fv(sun_uni_viewport, 1, viewport);
	glUniformMatrix4fv(sun_uni_inverse_projection, 1, 0, inverse_projection);
	glUniformMatrix4fv(sun_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(sun_uni_model_view, 1, 0, model_view);
	glUniform1i(sun_uni_depth_sampler, 0);
	glUniform1i(sun_uni_normal_sampler, 1);
	glUniform1i(sun_uni_albedo_sampler, 2);
	glUniform3fv(sun_uni_direction, 1, direction);
	glUniform3fv(sun_uni_color, 1, color);

	draw_fullscreen_quad();
}

/* Point light */

static const char *point_frag_src =
	"#version 120\n"
	"uniform vec2 Viewport;\n"
	"uniform mat4 InverseProjection;\n"
	"uniform sampler2D DepthSampler;\n"
	"uniform sampler2D NormalSampler;\n"
	"uniform sampler2D AlbedoSampler;\n"
	"uniform vec3 LightPosition;\n"
	"uniform vec3 LightColor;\n"
	"const float Shininess = 64.0;\n"
	"void main() {\n"
	"	vec2 texcoord = gl_FragCoord.xy / Viewport.xy;\n"
	"	float depth = texture2D(DepthSampler, texcoord).x;\n"
	"	vec4 pos_ndc = 2.0 * vec4(texcoord, depth, 1.0) - 1.0;\n"
	"	vec4 pos_hom = InverseProjection * pos_ndc;\n"
	"	vec3 position = pos_hom.xyz / pos_hom.w;\n"
	"	vec3 normal = texture2D(NormalSampler, texcoord).xyz;\n"
	"	vec4 albedo = texture2D(AlbedoSampler, texcoord);\n"

	"	vec3 direction = LightPosition - position;\n"
	"	float distance2 = dot(direction, direction);\n"
	"	float attenuation = 1.0 / distance2;\n"
	"	vec3 L = normalize(LightPosition - position);\n"
	"	float diffuse = max(dot(normal, L), 0.0) * 10 * attenuation;\n"

	"	vec3 V = normalize(-position);\n"
	"	vec3 H = normalize(L + V);\n"
	"	float specular = pow(max(dot(normal, H), 0.0), Shininess) * attenuation;\n"

	"	gl_FragColor = vec4((albedo.rgb * diffuse + albedo.a * specular) * LightColor, 1.0);\n"
	"}\n"
;

static int point_prog = 0;
static int point_uni_viewport;
static int point_uni_inverse_projection;
static int point_uni_projection;
static int point_uni_model_view;
static int point_uni_depth_sampler;
static int point_uni_normal_sampler;
static int point_uni_albedo_sampler;
static int point_uni_position;
static int point_uni_color;

void render_point_light(mat4 projection, mat4 model_view, vec3 point_position, vec3 color)
{
	mat4 inverse_projection;
	vec2 viewport;
	vec3 position;

	if (!point_prog) {
		point_prog = compile_shader(quad_vert_src, point_frag_src);
		point_uni_viewport = glGetUniformLocation(point_prog, "Viewport");
		point_uni_inverse_projection = glGetUniformLocation(point_prog, "InverseProjection");
		point_uni_projection = glGetUniformLocation(point_prog, "Projection");
		point_uni_model_view = glGetUniformLocation(point_prog, "ModelView");
		point_uni_depth_sampler = glGetUniformLocation(point_prog, "DepthSampler");
		point_uni_normal_sampler = glGetUniformLocation(point_prog, "NormalSampler");
		point_uni_albedo_sampler = glGetUniformLocation(point_prog, "AlbedoSampler");
		point_uni_position = glGetUniformLocation(point_prog, "LightPosition");
		point_uni_color = glGetUniformLocation(point_prog, "LightColor");
	}

	mat_invert(inverse_projection, projection);

	viewport[0] = fbo_w;
	viewport[1] = fbo_h;

	mat_vec_mul(position, model_view, point_position);

	glUseProgram(point_prog);
	glUniform2fv(point_uni_viewport, 1, viewport);
	glUniformMatrix4fv(point_uni_inverse_projection, 1, 0, inverse_projection);
	glUniformMatrix4fv(point_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(point_uni_model_view, 1, 0, model_view);
	glUniform1i(point_uni_depth_sampler, 0);
	glUniform1i(point_uni_normal_sampler, 1);
	glUniform1i(point_uni_albedo_sampler, 2);
	glUniform3fv(point_uni_position, 1, position);
	glUniform3fv(point_uni_color, 1, color);

	draw_fullscreen_quad();
}

/* draw model */

static const char *static_vert_src =
	"#version 120\n"
	"uniform mat4 Projection;\n"
	"uniform mat4 ModelView;\n"
	"attribute vec4 att_Position;\n"
	"attribute vec3 att_Normal;\n"
	"attribute vec2 att_TexCoord;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	gl_Position = Projection * ModelView * att_Position;\n"
	"	vec4 normal = ModelView * vec4(att_Normal, 0.0);\n"
	"	var_Normal = normalize(normal.xyz);\n"
	"	var_TexCoord = att_TexCoord;\n"
	"}\n"
;

static const char *static_frag_src =
	"#version 120\n"
	"uniform sampler2D SamplerDiffuse;\n"
	"uniform sampler2D SamplerSpecular;\n"
	"varying vec3 var_Normal;\n"
	"varying vec2 var_TexCoord;\n"
	"void main() {\n"
	"	vec4 albedo = texture2D(SamplerDiffuse, var_TexCoord);\n"
	"	vec4 gloss = texture2D(SamplerSpecular, var_TexCoord);\n"
	"	vec3 normal = normalize(var_Normal);\n"
	"	if (albedo.a < 0.2) discard;\n"
	"	gl_FragData[0] = vec4(normal.xyz, 0);\n"
	"	gl_FragData[1] = vec4(albedo.rgb, gloss.x);\n"
	"}\n"
;

static int static_prog = 0;
static int static_uni_projection;
static int static_uni_model_view;
static int static_uni_sampler_diffuse;
static int static_uni_sampler_specular;

void render_model(struct model *model, mat4 projection, mat4 model_view)
{
	int i;

	if (!model)
		return;

	if (!static_prog) {
		static_prog = compile_shader(static_vert_src, static_frag_src);
		static_uni_projection = glGetUniformLocation(static_prog, "Projection");
		static_uni_model_view = glGetUniformLocation(static_prog, "ModelView");
		static_uni_sampler_diffuse = glGetUniformLocation(static_prog, "SamplerDiffuse");
		static_uni_sampler_specular = glGetUniformLocation(static_prog, "SamplerSpecular");
	}

	glUseProgram(static_prog);
	glUniformMatrix4fv(static_uni_projection, 1, 0, projection);
	glUniformMatrix4fv(static_uni_model_view, 1, 0, model_view);
	glUniform1i(static_uni_sampler_diffuse, 0);
	glUniform1i(static_uni_sampler_specular, 1);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	for (i = 0; i < model->mesh_count; i++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].diffuse);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, model->mesh[i].specular);
		glActiveTexture(GL_TEXTURE0);
		glDrawElements(GL_TRIANGLES, model->mesh[i].count, GL_UNSIGNED_SHORT, (void*)(model->mesh[i].first * 2));
	}

	glBindVertexArray(0);
	glUseProgram(0);
}
