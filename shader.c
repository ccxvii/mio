#include "mio.h"

static char *load_source(char *filename)
{
	char *data;
	int len;
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "error: cannot open '%s'\n", filename);
		return NULL;
	}
	fseek(file, 0, 2);
	len = ftell(file);
	fseek(file, 0, 0);
	data = malloc(len+1);
	fread(data, 1, len, file);
	data[len] = 0;
	fclose(file);
	return data;
}

int compile_shader(char *vertfile, char *fragfile)
{
	char *vsrc, *fsrc;
	int prog, vs, fs, len;

	vsrc = load_source(vertfile);
	fsrc = load_source(fragfile);

	prog = glCreateProgram();
	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, (const char **)&vsrc, 0);
	glShaderSource(fs, 1, (const char **)&fsrc, 0);
	glCompileShader(vs);
	glCompileShader(fs);
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glValidateProgram(prog);

	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		char buf[800];
		glGetProgramInfoLog(prog, sizeof buf, &len, buf);
		fprintf(stderr, "%s", buf);
	}

	free(vsrc);
	free(fsrc);
	return prog;
}
