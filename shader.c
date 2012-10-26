#include "mio.h"

static void print_shader_log(char *kind, int shader)
{
	int len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetShaderInfoLog(shader, len, NULL, log);
	fprintf(stderr, "--- glsl %s shader compile results ---\n%s\n", kind, log);
	free(log);
}

static void print_program_log(int program)
{
	int len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetProgramInfoLog(program, len, NULL, log);
	fprintf(stderr, "--- glsl program link results ---\n%s\n", log);
	free(log);
}

int compile_shader(const char *vert_src, const char *frag_src)
{
	int status;

	if (!vert_src || !frag_src)
		return 0;

	int vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vert_src, NULL);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
	if (!status)
		print_shader_log("vertex", vert);

	int frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &frag_src, NULL);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
	if (!status)
		print_shader_log("fragment", frag);

	int prog = glCreateProgram();

	glBindAttribLocation(prog, ATT_POSITION, "att_Position");
	glBindAttribLocation(prog, ATT_TEXCOORD, "att_TexCoord");
	glBindAttribLocation(prog, ATT_NORMAL, "att_Normal");
	glBindAttribLocation(prog, ATT_TANGENT, "att_Tangent");
	glBindAttribLocation(prog, ATT_BLEND_INDEX, "att_BlendIndex");
	glBindAttribLocation(prog, ATT_BLEND_WEIGHT, "att_BlendWeight");
	glBindAttribLocation(prog, ATT_COLOR, "att_Color");

	glBindFragDataLocation(prog, FRAG_COLOR, "frag_Color");
	glBindFragDataLocation(prog, FRAG_NORMAL, "frag_Normal");
	glBindFragDataLocation(prog, FRAG_ALBEDO, "frag_Albedo");

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status)
		print_program_log(prog);

//	print_shader_log("vertex", vert);
//	print_shader_log("fragment", frag);
//	print_program_log(prog);

	glDeleteShader(vert);
	glDeleteShader(frag);

	return prog;
}
