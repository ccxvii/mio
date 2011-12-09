#include "mio.h"

static void print_shader_log(char *kind, int shader)
{
	int len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetShaderInfoLog(shader, len, NULL, log);
	printf("--- cannot compile glsl %s shader ---\n%s---\n", kind, log);
	free(log);
}

static void print_program_log(int program)
{
	int len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetProgramInfoLog(program, len, NULL, log);
	printf("--- cannot link glsl program ---\n%s---\n", log);
	free(log);
}

int compile_shader(const char *vert_src, const char *frag_src)
{
	int status;

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
	glBindAttribLocation(prog, ATT_NORMAL, "att_Normal");
	glBindAttribLocation(prog, ATT_TEXCOORD, "att_TexCoord");
	glBindAttribLocation(prog, ATT_COLOR, "att_Color");
	glBindAttribLocation(prog, ATT_BLEND_INDEX, "att_BlendIndex");
	glBindAttribLocation(prog, ATT_BLEND_WEIGHT, "att_BlendWeight");
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status)
		print_program_log(prog);

	glDeleteShader(vert);
	glDeleteShader(frag);

	return prog;
}
