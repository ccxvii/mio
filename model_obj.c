#include "mio.h"

#define SEP " \t\r\n"
#define MAXMESH 256
#define MAXNAME 80

struct floatarray {
	int len, cap;
	float *data;
};

struct intarray {
	int len, cap;
	unsigned short *data;
};

static struct {
	char name[MAXNAME];
	int texture;
	int repeat;
} material[MAXMESH];

static int material_count = 0;
static int material_current = 0;

// temp buffers are global so we can reuse them between models
static struct floatarray position = { 0, 0, NULL };
static struct floatarray texcoord = { 0, 0, NULL };
static struct floatarray normal = { 0, 0, NULL };
static struct floatarray vertex = { 0, 0, NULL };
static struct intarray element = { 0, 0, NULL };

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

static void add_position(float x, float y, float z)
{
	push_float(&position, x);
	push_float(&position, -z);
	push_float(&position, y);
}

static void add_texcoord(float u, float v)
{
	push_float(&texcoord, u);
	push_float(&texcoord, 1-v);
}

static void add_normal(float x, float y, float z)
{
	push_float(&normal, x);
	push_float(&normal, -z);
	push_float(&normal, y);
}

static int add_vertex_imp(float v[8])
{
	int i;
	for (i = vertex.len - 8; i >= 0; i -= 8)
		if (!memcmp(vertex.data + i, v, sizeof(float) * 8))
			return i / 8;
	for (i = 0; i < 8; i++)
		push_float(&vertex, v[i]);
	return vertex.len / 8 - 1;
}

static int add_vertex(int pi, int ti, int ni)
{
	float v[8];
	v[0] = position.data[pi * 3 + 0];
	v[1] = position.data[pi * 3 + 1];
	v[2] = position.data[pi * 3 + 2];
	v[3] = ti >= 0 && ti < texcoord.len ? texcoord.data[ti * 2 + 0] : 0;
	v[4] = ti >= 0 && ti < texcoord.len ? texcoord.data[ti * 2 + 1] : 0;
	v[5] = ni >= 0 && ni < normal.len ? normal.data[ni * 3 + 0] : 0;
	v[6] = ni >= 0 && ni < normal.len ? normal.data[ni * 3 + 1] : 0;
	v[7] = ni >= 0 && ni < normal.len ? normal.data[ni * 3 + 2] : 1;
	if (v[3] < 0 || v[3] > 1 || v[4] < 0 || v[4] > 1)
		material[material_current].repeat = 1;
	return add_vertex_imp(v);
}

static void add_triangle(
	int vp0, int vt0, int vn0,
	int vp1, int vt1, int vn1,
	int vp2, int vt2, int vn2)
{
	push_int(&element, add_vertex(vp0, vt0, vn0));
	push_int(&element, add_vertex(vp1, vt1, vn1));
	push_int(&element, add_vertex(vp2, vt2, vn2));
}

static void load_material(char *dirname, char *mtllib)
{
	char filename[1024];
	char line[200];
	char *s;
	FILE *fp;

	strlcpy(filename, dirname, sizeof filename);
	strlcat(filename, "/", sizeof filename);
	strlcat(filename, mtllib, sizeof filename);

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "error: cannot load material library '%s'\n", filename);
		return;
	}

	while (1) {
		if (!fgets(line, sizeof line, fp))
			break;
		s = strtok(line, SEP);
		if (!s) {
			continue;
		} else if (!strcmp(s, "newmtl")) {
			s = strtok(NULL, SEP);
			if (s) {
				strlcpy(material[material_count].name, s, MAXNAME);
				material[material_count].texture = 0;
				material[material_count].repeat = 0;
				material_count++;
			}
		} else if (!strcmp(s, "map_Kd")) {
			s = strtok(NULL, SEP);
			if (s && material_count > 0) {
				strlcpy(filename, dirname, sizeof filename);
				strlcat(filename, "/", sizeof filename);
				strlcat(filename, s, sizeof filename);
				material[material_count-1].texture = load_texture(0, filename, 1);
			}
		}
	}

	fclose(fp);
}

int set_material(char *matname)
{
	int i;
	for (i = 0; i < material_count; i++) {
		if (!strcmp(material[i].name, matname)) {
			material_current = i;
			return material[i].texture;
		}
	}
	material_current = i;
	return 0;
}

static void splitfv(char *buf, int *vpp, int *vtp, int *vnp)
{
	char tmp[200], *p, *vp, *vt, *vn;
	strlcpy(tmp, buf, sizeof tmp);
	p = tmp;
	vp = strsep(&p, "/");
	vt = strsep(&p, "/");
	vn = strsep(&p, "/");
	*vpp = vp && vp[0] ? atoi(vp) - 1 : 0;
	*vtp = vt && vt[0] ? atoi(vt) - 1 : 0;
	*vnp = vn && vn[0] ? atoi(vn) - 1 : 0;
}

struct model *load_obj_model(char *filename)
{
	char dirname[1024];
	char line[200];
	struct model *model;
	struct mesh meshbuf[MAXMESH], *mesh = NULL;
	int mesh_count = 0;
	int fvp[20], fvt[20], fvn[20];
	char *p, *s;
	int i, n;
	FILE *fp;

	printf("loading obj model '%s'\n", filename);

	strlcpy(dirname, filename, sizeof dirname);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strlcpy(dirname, ".", sizeof dirname);

	material_count = 0;
	position.len = 0;
	texcoord.len = 0;
	normal.len = 0;
	element.len = 0;

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
		} else if (!strcmp(s, "v")) {
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
		} else if (!strcmp(s, "f")) {
			n = 0;
			s = strtok(NULL, SEP);
			while (s) {
				if (*s) {
					splitfv(s, fvp+n, fvt+n, fvn+n);
					n++;
				}
				s = strtok(NULL, SEP);
			}
			for (i = 1; i < n - 1; i++) {
				add_triangle(fvp[0], fvt[0], fvn[0],
					fvp[i], fvt[i], fvn[i],
					fvp[i+1], fvt[i+1], fvn[i+1]);
			}
		} else if (!strcmp(s, "mtllib")) {
			s = strtok(NULL, SEP);
			load_material(dirname, s);
		} else if (!strcmp(s, "usemtl")) {
			s = strtok(NULL, SEP);
			if (mesh) {
				mesh->count = element.len - mesh->first;
				if (mesh->count == 0)
					mesh_count--;
			}
			mesh = &meshbuf[mesh_count++];
			mesh->texture = s ? set_material(s) : 0;
			mesh->first = element.len;
			mesh->count = 0;
		}
	}

	if (mesh) {
		mesh->count = element.len - mesh->first;
		if (mesh->count == 0)
			mesh_count--;
	}

	if (mesh_count == 0) {
		mesh = meshbuf;
		mesh->texture = 0;
		mesh->first = 0;
		mesh->count = element.len;
		mesh_count = 1;
	}

	fclose(fp);

	printf("\t%d meshes; %d vertices; %d triangles\n", mesh_count, vertex.len/8, element.len/3);

	for (i = 0; i < material_count; i++) {
		glBindTexture(GL_TEXTURE_2D, material[i].texture);
		if (material[i].repeat) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	model = malloc(sizeof *model);
	memset(model, 0, sizeof *model);

	model->mesh_count = mesh_count;
	model->mesh = malloc(mesh_count * sizeof(struct mesh));
	memcpy(model->mesh, meshbuf, mesh_count * sizeof(struct mesh));

	glGenVertexArrays(1, &model->vao);
	glGenBuffers(1, &model->vbo);
	glGenBuffers(1, &model->ibo);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	glBufferData(GL_ARRAY_BUFFER, vertex.len * 4, vertex.data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(ATT_POSITION);
	glEnableVertexAttribArray(ATT_TEXCOORD);
	glEnableVertexAttribArray(ATT_NORMAL);
	glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 32, (void*)0);
	glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 32, (void*)12);
	glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 32, (void*)20);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, element.len * 2, element.data, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return model;
}
