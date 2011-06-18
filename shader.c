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

struct program *compile_shader(char *vertfile, char *fragfile)
{
	struct program *p;
	char log[800];
	char *vsrc, *fsrc;
	int prog, vs, fs, len;

	vsrc = load_source(vertfile);
	fsrc = load_source(fragfile);

	if (!vsrc || !fsrc)
		return 0;

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

	free(vsrc);
	free(fsrc);

	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	glBindAttribLocation(prog, ATT_POSITION, "in_Position");
	glBindAttribLocation(prog, ATT_TEXCOORD, "in_TexCoord");
	glBindAttribLocation(prog, ATT_NORMAL, "in_Normal");
	glBindAttribLocation(prog, ATT_TANGENT, "in_Tangent");
	glBindAttribLocation(prog, ATT_BLEND_INDEX, "in_BlendIndex");
	glBindAttribLocation(prog, ATT_BLEND_WEIGHT, "in_BlendWeight");
	glBindAttribLocation(prog, ATT_COLOR, "in_Color");

	glLinkProgram(prog);

	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 1) {
		glGetProgramInfoLog(prog, sizeof log, &len, log);
		fprintf(stderr, "%s/%s:\n%s", vertfile, fragfile, log);
	}

	p = malloc(sizeof *p);
	p->program = prog;
	p->vert_shader = vs;
	p->frag_shader = fs;

	p->model_view = glGetUniformLocation(p->program, "ModelView");
	p->projection = glGetUniformLocation(p->program, "Projection");

	p->light_dir = glGetUniformLocation(p->program, "LightDirection");
	p->light_ambient = glGetUniformLocation(p->program, "LightAmbient");
	p->light_diffuse = glGetUniformLocation(p->program, "LightDiffuse");
	p->light_specular = glGetUniformLocation(p->program, "LightSpecular");

	p->wind = glGetUniformLocation(p->program, "Wind");
	p->phase = glGetUniformLocation(p->program, "Phase");
	p->bone_matrix = glGetUniformLocation(p->program, "BoneMatrix");

	return p;
}
