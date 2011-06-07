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
	char log[800];
	char *vsrc, *fsrc;
	int prog, vs, fs, len;

	vsrc = load_source(vertfile);
	fsrc = load_source(fragfile);

	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const char **)&vsrc, 0);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);
	if (len > 1) {
		glGetShaderInfoLog(vs, sizeof log, &len, log);
		fprintf(stderr, "%s:\n%s", vertfile, log);
	}

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const char **)&fsrc, 0);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &len);
	if (len > 1) {
		glGetShaderInfoLog(fs, sizeof log, &len, log);
		fprintf(stderr, "%s:\n%s", fragfile, log);
	}

	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 1) {
		glGetProgramInfoLog(prog, sizeof log, &len, log);
		fprintf(stderr, "%s/%s:\n%s", vertfile, fragfile, log);
	}

	free(vsrc);
	free(fsrc);
	return prog;
}
