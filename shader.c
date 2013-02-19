#include "mio.h"

#ifdef __APPLE__
#define GLSL_VERT_PROLOG "#version 150\n"
#define GLSL_FRAG_PROLOG "#version 150\n"
#else
#define GLSL_VERT_PROLOG "#version 130\n"
#define GLSL_FRAG_PROLOG "#version 130\n"
#endif

const char *gl_error_string(GLenum code)
{
#define CASE(E) case E: return #E; break
	switch (code)
	{
	/* glGetError */
	CASE(GL_NO_ERROR);
	CASE(GL_INVALID_ENUM);
	CASE(GL_INVALID_VALUE);
	CASE(GL_INVALID_OPERATION);
	CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
	CASE(GL_OUT_OF_MEMORY);
	CASE(GL_STACK_UNDERFLOW);
	CASE(GL_STACK_OVERFLOW);

	/* glCheckFramebufferStatus */
	CASE(GL_FRAMEBUFFER_COMPLETE);
	CASE(GL_FRAMEBUFFER_UNDEFINED);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
	CASE(GL_FRAMEBUFFER_UNSUPPORTED);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
	CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);

	default: return "(unknown)";
	}
#undef CASE
}

void gl_assert(const char *msg)
{
	int code = glGetError();
	if (code != GL_NO_ERROR) {
		warn("glGetError(%s): %s", msg, gl_error_string(code));
	}
}

void gl_assert_framebuffer(GLenum target, const char *msg)
{
	int code = glCheckFramebufferStatus(target);
	if (code != GL_FRAMEBUFFER_COMPLETE) {
		warn("glCheckFramebufferStatus(%s): %s", msg, gl_error_string(code));
		warn("Fatal error!");
		exit(1);
	}
}

static void print_shader_log(char *kind, int shader)
{
	int len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetShaderInfoLog(shader, len, NULL, log);
	warn("--- glsl %s shader compile results ---\n%s", kind, log);
	free(log);
}

static void print_program_log(int program)
{
	int len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetProgramInfoLog(program, len, NULL, log);
	warn("--- glsl program link results ---\n%s", log);
	free(log);
}

int compile_shader(const char *vert_src, const char *frag_src)
{
	const char *vert_src_list[2];
	const char *frag_src_list[2];
	int status;

	if (!vert_src || !frag_src)
		return 0;

	vert_src_list[0] = GLSL_VERT_PROLOG;
	vert_src_list[1] = vert_src;

	int vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, nelem(vert_src_list), vert_src_list, NULL);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
	if (!status)
		print_shader_log("vertex", vert);

	frag_src_list[0] = GLSL_FRAG_PROLOG;
	frag_src_list[1] = frag_src;

	int frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, nelem(frag_src_list), frag_src_list, NULL);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
	if (!status)
		print_shader_log("fragment", frag);

	int prog = glCreateProgram();

	glBindAttribLocation(prog, ATT_POSITION, "att_position");
	glBindAttribLocation(prog, ATT_NORMAL, "att_normal");
	glBindAttribLocation(prog, ATT_TANGENT, "att_tangent");
	glBindAttribLocation(prog, ATT_TEXCOORD, "att_texcoord");
	glBindAttribLocation(prog, ATT_COLOR, "att_color");
	glBindAttribLocation(prog, ATT_BLEND_INDEX, "att_blend_index");
	glBindAttribLocation(prog, ATT_BLEND_WEIGHT, "att_blend_weight");
	glBindAttribLocation(prog, ATT_LIGHTMAP, "att_lightmap");
	glBindAttribLocation(prog, ATT_SPLAT, "att_splat");
	glBindAttribLocation(prog, ATT_WIND, "att_wind");

	glBindFragDataLocation(prog, FRAG_COLOR, "frag_color");
	glBindFragDataLocation(prog, FRAG_NORMAL, "frag_normal");
	glBindFragDataLocation(prog, FRAG_ALBEDO, "frag_albedo");

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status)
		print_program_log(prog);

	glDetachShader(prog, vert);
	glDetachShader(prog, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);

	glUseProgram(prog);
	glUniform1i(glGetUniformLocation(prog, "map_color"), MAP_COLOR - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_gloss"), MAP_GLOSS - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_normal"), MAP_NORMAL - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_shadow"), MAP_SHADOW - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_depth"), MAP_DEPTH - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_emission"), MAP_EMISSION - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_light"), MAP_LIGHT - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(prog, "map_splat"), MAP_SPLAT - GL_TEXTURE0);

	return prog;
}
