#include "mio.h"

#define SEP " \t\r\n"

struct floatarray {
	int len, cap;
	float *data;
};

struct intarray {
	int len, cap;
	unsigned short *data;
};

struct partarray {
	int len, cap;
	struct part *data;
};

// temp buffers are global so we can reuse them between models
static struct floatarray position = { 0, 0, NULL };
static struct floatarray normal = { 0, 0, NULL };
static struct floatarray texcoord = { 0, 0, NULL };
static struct floatarray vertex = { 0, 0, NULL };
static struct intarray element = { 0, 0, NULL };
static struct partarray part = { 0, 0, NULL };

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

static inline void push_part(struct partarray *a, int first, int last, int material)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len].first = first;
	a->data[a->len].count = last - first;
	a->data[a->len].material = material;
	a->len++;
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

static char *abspath(char *path, const char *dirname, const char *filename, int maxpath)
{
	if (dirname[0]) {
		strlcpy(path, dirname, maxpath);
		strlcat(path, "/", maxpath);
		strlcat(path, filename, maxpath);
	} else {
		strlcpy(path, filename, maxpath);
	}
	return path;
}

static int mtl_count = 0;

static struct {
	char name[80];
	int material;
} mtl_map[256];

static void mtllib(char *dirname, char *filename)
{
	char path[1024];
	char *line, *next;
	unsigned char *data;
	int len;
	char *s;

	data = load_file(abspath(path, dirname, filename, sizeof path), &len);
	if (!data) {
		warn("cannot load material library: '%s'", filename);
		return;
	}

	data[len-1] = 0; /* over-write final newline to zero-terminate */

	for (line = (char*)data; line; line = next) {
		next = strchr(line, '\n');
		if (next)
			*next++ = 0;

		s = strtok(line, SEP);
		if (!s) {
			continue;
		} else if (!strcmp(s, "newmtl")) {
			s = strtok(NULL, SEP);
			if (s) {
				strlcpy(mtl_map[mtl_count].name, s, sizeof mtl_map[0].name);
				mtl_map[mtl_count].material = 0;
				mtl_count++;
			}
		} else if (!strcmp(s, "map_Kd")) {
			s = strtok(NULL, SEP);
			if (s && mtl_count > 0) {
				mtl_map[mtl_count-1].material = load_texture(abspath(path, dirname, s, sizeof path), 1);
			}
		}
	}

	free(data);
}

int usemtl(char *matname)
{
	int i;
	for (i = 0; i < mtl_count; i++)
		if (!strcmp(mtl_map[i].name, matname))
			return mtl_map[i].material;
	return 0;
}

static void splitfv(char *buf, int *vpp, int *vnp, int *vtp)
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

struct model *load_obj_from_memory(const char *filename, unsigned char *data, int len)
{
	char dirname[1024];
	char *line, *next, *p, *s;
	struct model *model;
	struct mesh *mesh;
	int fvp[20], fvt[20], fvn[20];
	int first, material;
	int i, n;

	printf("loading obj model '%s'\n", filename);

	strlcpy(dirname, filename, sizeof dirname);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strlcpy(dirname, "", sizeof dirname);

	mtl_count = 0;
	position.len = 0;
	texcoord.len = 0;
	normal.len = 0;
	element.len = 0;
	part.len = 0;

	first = 0;
	material = 0;

	data[len-1] = 0; /* over-write final newline to zero-terminate */

	for (line = (char*)data; line; line = next) {
		next = strchr(line, '\n');
		if (next)
			*next++ = 0;

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
					splitfv(s, fvp+n, fvn+n, fvt+n);
					n++;
				}
				s = strtok(NULL, SEP);
			}
			for (i = 1; i < n - 1; i++) {
				add_triangle(fvp[0], fvn[0], fvt[0],
					fvp[i], fvn[i], fvt[i],
					fvp[i+1], fvn[i+1], fvt[i+1]);
			}
		} else if (!strcmp(s, "mtllib")) {
			s = strtok(NULL, SEP);
			mtllib(dirname, s);
		} else if (!strcmp(s, "usemtl")) {
			if (element.len > first)
				push_part(&part, first, element.len, material);
			s = strtok(NULL, SEP);
			material = usemtl(s);
			first = element.len;
		}
	}

	if (element.len > first)
		push_part(&part, first, element.len, material);

	printf("\t%d parts; %d vertices; %d triangles", part.len, vertex.len/8, element.len/3);

	mesh = malloc(sizeof(struct mesh));
	mesh->tag = TAG_MESH;
	mesh->enabled = 1<<ATT_POSITION | 1<<ATT_NORMAL | 1<<ATT_TEXCOORD;
	mesh->skel = NULL;
	mesh->inv_bind_matrix = NULL;
	mesh->count = part.len;
	mesh->part = malloc(part.len * sizeof(struct part));
	memcpy(mesh->part, part.data, part.len * sizeof(struct part));

	glGenVertexArrays(1, &mesh->vao);
	glGenBuffers(1, &mesh->vbo);
	glGenBuffers(1, &mesh->ibo);

	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

	glBufferData(GL_ARRAY_BUFFER, vertex.len * 4, vertex.data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(ATT_POSITION);
	glEnableVertexAttribArray(ATT_NORMAL);
	glEnableVertexAttribArray(ATT_TEXCOORD);
	glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 32, (void*)0);
	glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 32, (void*)20);
	glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 32, (void*)12);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, element.len * 2, element.data, GL_STATIC_DRAW);

	model = malloc(sizeof *model);
	model->skel = NULL;
	model->mesh = mesh;
	model->anim = NULL;
	return model;
}
