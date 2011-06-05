#include "mio.h"

#define SEP " \t\r\n"

struct material {
	char name[80];
	int texture;
	struct material *next;
};

struct triangle {
	int vp[3];
	int vt[3];
	int vn[3];
};

struct array {
	int len, cap;
	float *data;
};

struct mesh {
	struct material *material;
	int len, cap;
	struct triangle *tri;
	struct mesh *next;
	float *vba;
	unsigned int vbo;
};

struct model {
	struct material *material;
	struct array position;
	struct array texcoord;
	struct array normal;
	struct mesh *mesh;
};

static inline void push(struct array *a, float v)
{
	if (a->len + 1 >= a->cap) {
		a->cap = 600 + a->cap * 2;
		a->data = realloc(a->data, a->cap * sizeof(*a->data));
	}
	a->data[a->len++] = v;
}

static void add_position(struct model *model, float x, float y, float z)
{
	push(&model->position, x);
	push(&model->position, -z);
	push(&model->position, y);
}

static void add_texcoord(struct model *model, float u, float v)
{
	push(&model->texcoord, u);
	push(&model->texcoord, 1-v);
}

static void add_normal(struct model *model, float x, float y, float z)
{
	push(&model->normal, x);
	push(&model->normal, -z);
	push(&model->normal, y);
}

static void add_triangle(struct mesh *mesh,
	int vp0, int vt0, int vn0,
	int vp1, int vt1, int vn1,
	int vp2, int vt2, int vn2)
{
	if (!mesh)
		return;
	if (mesh->len + 1 >= mesh->cap) {
		mesh->cap = 600 + mesh->cap * 2;
		mesh->tri = realloc(mesh->tri, mesh->cap * sizeof(*mesh->tri));
	}
	mesh->tri[mesh->len].vp[0] = vp0 * 3;
	mesh->tri[mesh->len].vp[1] = vp1 * 3;
	mesh->tri[mesh->len].vp[2] = vp2 * 3;
	mesh->tri[mesh->len].vt[0] = vt0 * 2;
	mesh->tri[mesh->len].vt[1] = vt1 * 2;
	mesh->tri[mesh->len].vt[2] = vt2 * 2;
	mesh->tri[mesh->len].vn[0] = vn0 * 3;
	mesh->tri[mesh->len].vn[1] = vn1 * 3;
	mesh->tri[mesh->len].vn[2] = vn2 * 3;
	mesh->len++;
}

static struct material *load_material(char *dirname, char *mtllib)
{
	char filename[1024];
	char line[200];
	char *s;
	struct material *head = NULL, *mat = NULL;
	FILE *fp;

	strlcpy(filename, dirname, sizeof filename);
	strlcat(filename, "/", sizeof filename);
	strlcat(filename, mtllib, sizeof filename);

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "error: cannot load material library '%s'\n", filename);
		return NULL;
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
				mat = malloc(sizeof(struct material));
				strlcpy(mat->name, s, sizeof mat->name);
				mat->texture = 0;
				mat->next = head;
				head = mat;
			}
		} else if (!strcmp(s, "map_Kd")) {
			s = strtok(NULL, SEP);
			if (s && mat) {
				strlcpy(filename, dirname, sizeof filename);
				strlcat(filename, "/", sizeof filename);
				strlcat(filename, s, sizeof filename);
				mat->texture = load_texture(0, filename);
				printf("\ttexture %s = %d\n", s, mat->texture);
			}
		}
	}

	fclose(fp);
	return head;
}

static void splitfv(char *buf, int *vpp, int *vtp, int *vnp)
{
	char tmp[200], *p, *vp, *vt, *vn;
	strcpy(tmp, buf);
	p = tmp;
	vp = strsep(&p, "/");
	vt = strsep(&p, "/");
	vn = strsep(&p, "/");
	*vpp = vp ? atoi(vp) - 1 : -1;
	*vtp = vt ? atoi(vt) - 1 : -1;
	*vnp = vn ? atoi(vn) - 1 : -1;
}

struct material *find_material(struct model *model, char *matname)
{
	struct material *mat;
	for (mat = model->material; mat; mat = mat->next)
		if (!strcmp(mat->name, matname))
			return mat;
	return NULL;
}

struct mesh *find_mesh(struct model *model, char *matname)
{
	struct material *material = find_material(model, matname);
	struct mesh *mesh;
	for (mesh = model->mesh; mesh; mesh = mesh->next)
		if (mesh->material == material)
			return mesh;
	mesh = malloc(sizeof(struct mesh));
	mesh->material = material;
	mesh->len = mesh->cap = 0;
	mesh->tri = 0;
	mesh->next = model->mesh;
	mesh->vba = 0;
	mesh->vbo = 0;
	model->mesh = mesh;
	return mesh;
}

struct model *load_obj_model(char *filename)
{
	char dirname[1024];
	char line[200];
	struct model *model;
	struct mesh *curmesh = NULL;
	int fvp[20], fvt[20], fvn[20];
	char *p, *s;
	int i, n;
	FILE *fp;

	strlcpy(dirname, filename, sizeof dirname);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strlcpy(dirname, ".", sizeof dirname);

	printf("loading obj model: %s\n", filename);

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "error: cannot load model '%s'\n", filename);
		return NULL;
	}

	model = malloc(sizeof(struct model));
	memset(model, 0, sizeof(struct model));

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
			add_position(model, atof(x), atof(y), atof(z));
		} else if (!strcmp(s, "vt")) {
			char *u = strtok(NULL, SEP);
			char *v = strtok(NULL, SEP);
			add_texcoord(model, atof(u), atof(v));
		} else if (!strcmp(s, "vn")) {
			char *x = strtok(NULL, SEP);
			char *y = strtok(NULL, SEP);
			char *z = strtok(NULL, SEP);
			add_normal(model, atof(x), atof(y), atof(z));
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
				add_triangle(curmesh,
					fvp[0], fvt[0], fvn[0],
					fvp[i], fvt[i], fvn[i],
					fvp[i+1], fvt[i+1], fvn[i+1]);
			}
		} else if (!strcmp(s, "mtllib")) {
			s = strtok(NULL, SEP);
			model->material = load_material(dirname, s);
		} else if (!strcmp(s, "usemtl")) {
			s = strtok(NULL, SEP);
			curmesh = find_mesh(model, s);
		}
	}

	fclose(fp);

	printf("\t%d vertices, %d texcoords, %d normals\n",
			model->position.len,
			model->texcoord.len,
			model->normal.len);
	for (curmesh = model->mesh; curmesh; curmesh = curmesh->next)
		printf("\tmesh material=%s texture=%d triangles=%d\n",
				curmesh->material->name,
				curmesh->material->texture,
				curmesh->len);

	return model;
}

static inline float dot(float *u, float *v)
{
	return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

static inline void cross(float *u, float *v, float *out)
{
	out[0] = u[1]*v[2] - u[2]*v[1];
	out[1] = u[2]*v[0] - u[0]*v[2];
	out[2] = u[0]*v[1] - u[1]*v[0];
}

static inline void normalize(float *v)
{
	float l = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static inline void compute_face_normal(struct model *model, struct mesh *mesh, int t)
{
	float ba[3], ca[3], n[3];
	float *a = model->position.data + mesh->tri[t].vp[0];
	float *b = model->position.data + mesh->tri[t].vp[1];
	float *c = model->position.data + mesh->tri[t].vp[2];
	ba[0] = b[0] - a[0]; ba[1] = b[1] - a[1]; ba[2] = b[2] - a[2];
	ca[0] = c[0] - a[0]; ca[1] = c[1] - a[1]; ca[2] = c[2] - a[2];
	cross(ba, ca, n);
	normalize(n);
	glNormal3fv(n);
}

static void make_vbo(struct model *model, struct mesh *mesh)
{
	int i, k, v = 0, n = mesh->len * 3 * 8;
	mesh->vba = malloc(n * sizeof(float));
	for (i = 0; i < mesh->len; i++) {
		for (k = 0; k < 3; k++) {
			int pi = mesh->tri[i].vp[k];
			int ti = mesh->tri[i].vt[k];
			int ni = mesh->tri[i].vn[k];
			mesh->vba[v++] = model->position.data[pi+0];
			mesh->vba[v++] = model->position.data[pi+1];
			mesh->vba[v++] = model->position.data[pi+2];
			if (ti) {
				mesh->vba[v++] = model->texcoord.data[ti+0];
				mesh->vba[v++] = model->texcoord.data[ti+1];
			} else {
				mesh->vba[v++] = 0;
				mesh->vba[v++] = 0;
			}
			if (ni) {
				mesh->vba[v++] = model->normal.data[ni+0];
				mesh->vba[v++] = model->normal.data[ni+1];
				mesh->vba[v++] = model->normal.data[ni+2];
			} else {
				mesh->vba[v++] = 0;
				mesh->vba[v++] = 0;
				mesh->vba[v++] = 1;
			}
		}
	}

	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(float), mesh->vba, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_obj_model(struct model *model)
{
	struct mesh *mesh;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	for (mesh = model->mesh; mesh; mesh = mesh->next) {
		glBindTexture(GL_TEXTURE_2D, mesh->material->texture);
		if (!mesh->vbo)
			make_vbo(model, mesh);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
		glVertexPointer(3, GL_FLOAT, 8*4, (float*)0+0);
		glTexCoordPointer(2, GL_FLOAT, 8*4, (float*)0+3);
		glNormalPointer(GL_FLOAT, 8*4, (float*)0+5);
		glDrawArrays(GL_TRIANGLES, 0, mesh->len * 3);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}
