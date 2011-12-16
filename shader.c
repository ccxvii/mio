#include "mio.h"

unsigned char *load_file(char *filename, int *lenp)
{
	unsigned char *data;
	int len;
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "error: cannot open '%s'\n", filename);
		return NULL;
	}
	fseek(file, 0, 2);
	len = ftell(file);
	fseek(file, 0, 0);
	data = malloc(len + 1);
	fread(data, 1, len, file);
	fclose(file);
	if (lenp) *lenp = len;
	data[len] = 0; // nul-terminate in case it's a text file we use as a string
	return data;
}

static void print_shader_log(char *kind, int shader)
{
	int len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetShaderInfoLog(shader, len, NULL, log);
	printf("--- glsl %s shader compile results ---\n%s\n", kind, log);
	free(log);
}

static void print_program_log(int program)
{
	int len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	char *log = malloc(len + 1);
	glGetProgramInfoLog(program, len, NULL, log);
	printf("--- glsl program link results ---\n%s\n", log);
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
	glBindAttribLocation(prog, ATT_TEXCOORD, "att_TexCoord");
	glBindAttribLocation(prog, ATT_NORMAL, "att_Normal");
	glBindAttribLocation(prog, ATT_TANGENT, "att_Tangent");
	glBindAttribLocation(prog, ATT_BLEND_INDEX, "att_BlendIndex");
	glBindAttribLocation(prog, ATT_BLEND_WEIGHT, "att_BlendWeight");
	glBindAttribLocation(prog, ATT_COLOR, "att_Color");
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
