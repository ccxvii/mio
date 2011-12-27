#include "mio.h"

#define SEP " \t\r\n"
#define MAXMESH 256

struct floatarray {
	int len, cap;
	float *data;
};

struct intarray {
	int len, cap;
	unsigned short *data;
};

struct bytearray {
	int len, cap;
	unsigned char *data;
};

// temp buffers are global so we can reuse them between models
static struct floatarray position = { 0, 0, NULL };
static struct floatarray normal = { 0, 0, NULL };
static struct floatarray texcoord = { 0, 0, NULL };
static struct bytearray color = { 0, 0, NULL };
static struct bytearray blendindex = { 0, 0, NULL };
static struct bytearray blendweight = { 0, 0, NULL };
static struct intarray element = { 0, 0, NULL };

static int tile_s, tile_t;

static inline void push_float(struct floatarray *a, float v)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static inline void push_int(struct intarray *a, int v)
{
	assert(v >= 0 && v < 65535);
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static inline void push_byte(struct bytearray *a, int v)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static void add_position(float x, float y, float z)
{
	push_float(&position, x);
	push_float(&position, y);
	push_float(&position, z);
}

static void add_texcoord(float u, float v)
{
	if (u < 0 || u > 1) tile_s = 1;
	if (v < 0 || v > 1) tile_t = 1;
	push_float(&texcoord, u);
	push_float(&texcoord, v);
}

static void add_normal(float x, float y, float z)
{
	push_float(&normal, x);
	push_float(&normal, y);
	push_float(&normal, z);
}

static void add_color(float r, float g, float b, float a)
{
	push_byte(&color, r * 255);
	push_byte(&color, g * 255);
	push_byte(&color, b * 255);
	push_byte(&color, a * 255);
}

static void add_blend(int idx[4], float wgt[4])
{
	int i;
	for (i = 0; i < 4; i++) {
		push_byte(&blendindex, idx[i]);
		push_byte(&blendweight, wgt[i] * 255);
	}
}

static void add_triangle(int a, int b, int c)
{
	// flip triangle winding
	push_int(&element, c);
	push_int(&element, b);
	push_int(&element, a);
}

static int load_material(char *dirname, char *material)
{
	char filename[1024], *s;
	s = strrchr(material, '+');
	if (s) s++; else s = material;
	strlcpy(filename, dirname, sizeof filename);
	strlcat(filename, "/", sizeof filename);
	strlcat(filename, s, sizeof filename);
	strlcat(filename, ".png", sizeof filename);
	return load_texture(0, filename, 1);
}

struct model *load_iqe_model(char *filename)
{
	char dirname[1024];
	char line[200];
	struct model *model;
	struct mesh meshbuf[MAXMESH], *mesh = NULL;
	char bonename[MAXBONE][32];
	int boneparent[MAXBONE];
	struct pose posebuf[MAXBONE];
	int mesh_count = 0;
	int bone_count = 0;
	int pose_count = 0;
	int material = 0;
	int fm = 0;
	char *p, *s;
	int i;
	FILE *fp;

	printf("loading iqe model '%s'\n", filename);

	strlcpy(dirname, filename, sizeof dirname);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strlcpy(dirname, ".", sizeof dirname);

	position.len = 0;
	texcoord.len = 0;
	normal.len = 0;
	color.len = 0;
	blendindex.len = 0;
	blendweight.len = 0;
	element.len = 0;

	tile_s = tile_t = 0;

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "error: cannot load model '%s'\n", filename);
		return NULL;
	}

	while (1) {
		if (!fgets(line, sizeof line, fp))
			break;

		s = strtok(line, SEP);
		if (!s) {
			continue;
		} else if (!strcmp(s, "vp")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			add_position(atof(x), atof(y), atof(z));
		} else if (!strcmp(s, "vt")) {
			char *u = strtok(NULL, SEP);
			char *v = strtok(NULL, SEP);
			add_texcoord(atof(u), atof(v));
		} else if (!strcmp(s, "vn")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			add_normal(atof(x), atof(y), atof(z));
		} else if (!strcmp(s, "vc")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			char *w = strtok(NULL, SEP);
			add_color(atof(x), atof(y), atof(z), atof(w));
		} else if (!strcmp(s, "vb")) {
			int idx[4] = {0, 0, 0, 0};
			float wgt[4] = {1, 0, 0, 0};
			for (i = 0; i < 4; i++) {
				char *x = strtok(NULL, SEP);
				char *y = strtok(NULL, SEP);
				if (x && y) {
					idx[i] = atoi(x);
					wgt[i] = atof(y);
				} else break;
			}
			add_blend(idx, wgt);
		} else if (!strcmp(s, "fm")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			add_triangle(atoi(x)+fm, atoi(y)+fm, atoi(z)+fm);
		} else if (!strcmp(s, "fa")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			add_triangle(atoi(x), atoi(y), atoi(z));
		} else if (!strcmp(s, "mesh")) {
			if (mesh) {
				printf("mesh tile %d %d\n", tile_s, tile_t);
				glBindTexture(GL_TEXTURE_2D, mesh->texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tile_s ? GL_REPEAT : GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tile_t ? GL_REPEAT : GL_CLAMP_TO_EDGE);
				mesh->count = element.len - mesh->first;
				if (mesh->count == 0)
					mesh_count--;
			}
			mesh = &meshbuf[mesh_count++];
			mesh->texture = material;
			mesh->first = element.len;
			mesh->count = 0;
			fm = position.len / 3;
			tile_s = tile_t = 0;
		} else if (!strcmp(s, "material")) {
			s = strtok(NULL, SEP);
			material = load_material(dirname, s);
			if (mesh) {
				mesh->texture = material;
				mesh->alphatest = !!strstr(s, "alphatest+");
				mesh->alphagloss = !!strstr(s, "alphagloss+");
				mesh->unlit = !!strstr(s, "unlit+");
			}
		} else if (!strcmp(s, "joint")) {
			if (bone_count < MAXBONE) {
				char *name = strtok(NULL, SEP);
				char *parent = strtok(NULL, SEP);
				strlcpy(bonename[bone_count], name, sizeof bonename[0]);
				boneparent[bone_count] = atoi(parent);
				bone_count++;
			}
		} else if (!strcmp(s, "pq")) {
			if (pose_count < MAXBONE) {
				posebuf[pose_count].translate[0] = atof(strtok(NULL, SEP));
				posebuf[pose_count].translate[1] = atof(strtok(NULL, SEP));
				posebuf[pose_count].translate[2] = atof(strtok(NULL, SEP));
				posebuf[pose_count].rotate[0] = atof(strtok(NULL, SEP));
				posebuf[pose_count].rotate[1] = atof(strtok(NULL, SEP));
				posebuf[pose_count].rotate[2] = atof(strtok(NULL, SEP));
				posebuf[pose_count].rotate[3] = atof(strtok(NULL, SEP));
				char *sx = strtok(NULL, SEP);
				char *sy = strtok(NULL, SEP);
				char *sz = strtok(NULL, SEP);
				if (sx) {
					posebuf[pose_count].scale[0] = atof(sx);
					posebuf[pose_count].scale[1] = atof(sy);
					posebuf[pose_count].scale[2] = atof(sz);
				} else {
					posebuf[pose_count].scale[0] = 1;
					posebuf[pose_count].scale[1] = 1;
					posebuf[pose_count].scale[2] = 1;
				}
				pose_count++;
			}
		}
	}

	if (mesh) {
		printf("mesh tile %d %d\n", tile_s, tile_t);
		glBindTexture(GL_TEXTURE_2D, mesh->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tile_s ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tile_t ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		mesh->count = element.len - mesh->first;
		if (mesh->count == 0)
			mesh_count--;
	}

	if (mesh_count == 0) {
		mesh = meshbuf;
		mesh->texture = 0;
		mesh->alphatest = 1;
		mesh->alphagloss = 0;
		mesh->unlit = 0;
		mesh->first = 0;
		mesh->count = element.len;
		mesh_count = 1;
	}

	fclose(fp);

	printf("\t%d meshes; %d bones; %d vertices; %d triangles\n",
			mesh_count, bone_count, position.len/3, element.len/3);

	model = malloc(sizeof *model);
	memset(model, 0, sizeof *model);

	model->mesh_count = mesh_count;
	model->mesh = malloc(mesh_count * sizeof(struct mesh));
	memcpy(model->mesh, meshbuf, mesh_count * sizeof(struct mesh));

	int vertexcount = position.len / 3;
	int total = 12;
	if (normal.len / 3 == vertexcount) total += 12;
	if (texcoord.len / 2 == vertexcount) total += 8;
	if (color.len / 4 == vertexcount) total += 4;
	if (blendindex.len / 4 == vertexcount) total += 4;
	if (blendweight.len / 4 == vertexcount) total += 4;

	glGenVertexArrays(1, &model->vao);
	glGenBuffers(1, &model->vbo);
	glGenBuffers(1, &model->ibo);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	glBufferData(GL_ARRAY_BUFFER, vertexcount * total * 4, NULL, GL_STATIC_DRAW);

	glEnableVertexAttribArray(ATT_POSITION);
	glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 0, (void*)0);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexcount * 12, position.data);
	total = vertexcount * 12;

	if (normal.len / 3 == vertexcount) {
		glEnableVertexAttribArray(ATT_NORMAL);
		glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 0, (void*)total);
		glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 12, normal.data);
		total += vertexcount * 12;
	}
	if (texcoord.len / 2 == vertexcount) {
		glEnableVertexAttribArray(ATT_TEXCOORD);
		glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 0, (void*)total);
		glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 8, texcoord.data);
		total += vertexcount * 8;
	}
	if (color.len / 4 == vertexcount) {
		glEnableVertexAttribArray(ATT_COLOR);
		glVertexAttribPointer(ATT_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)total);
		glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, color.data);
		total += vertexcount * 4;
	}
	if (blendindex.len / 4 == vertexcount) {
		glEnableVertexAttribArray(ATT_BLEND_INDEX);
		glVertexAttribPointer(ATT_BLEND_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, (void*)total);
		glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, blendindex.data);
		total += vertexcount * 4;
	}
	if (blendweight.len / 4 == vertexcount) {
		glEnableVertexAttribArray(ATT_BLEND_WEIGHT);
		glVertexAttribPointer(ATT_BLEND_WEIGHT, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)total);
		glBufferSubData(GL_ARRAY_BUFFER, total, vertexcount * 4, blendweight.data);
		total += vertexcount * 4;
	}

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, element.len * 2, element.data, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	if (bone_count > 0 && pose_count >= bone_count) {
		model->bone_count = bone_count;
		memcpy(model->bone_name, bonename, sizeof bonename); // XXX careful of size
		memcpy(model->parent, boneparent, sizeof boneparent);
		memcpy(model->bind_pose, posebuf, sizeof posebuf);
		calc_pose_matrix(model->bind_matrix, model->bind_pose, model->bone_count);
		calc_abs_pose_matrix(model->abs_bind_matrix, model->bind_matrix, model->parent, model->bone_count);
		calc_inv_bind_matrix(model->inv_bind_matrix, model->abs_bind_matrix, model->bone_count);
	}

	return model;
}
