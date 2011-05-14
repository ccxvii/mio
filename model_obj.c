#include "mio.h"

struct material {
	char *name;
	int texture;
};

struct triangle {
	int v[3];
	int t[3];
};

struct model {
	char *name;
	int nm, nv, nvt, nvn, nf;
	float *xyz;
	float *uv;
	struct triangle *tri;
	struct material *mat;
};

static struct { int len, cap; int *buf; } g_v;
static struct { int len, cap; int *buf; } g_vt;
static struct { int len, cap; int *buf; } g_vn;
static struct { int len, cap; int *buf; } g_f;

static void addv(float x, float y, float z)
{
	printf("v %g %g %g\n", x, y, z);
}

static void addvt(float u, float v)
{
	printf("vt %g %g\n", u, v);
}

static void addvn(float x, float y, float z)
{
	printf("vn %g %g %g\n", x, y, z);
}

static void addf(int v0, int vt0, int vn0,
	int v1, int vt1, int vn1,
	int v2, int vt2, int vn2)
{
	printf("f %d/%d/%d %d/%d/%d %d/%d/%d\n",
		v0+1, vt0+1, vn0+1,
		v1+1, vt1+1, vn1+1,
		v2+1, vt2+1, vn2+1);
}

static void load_material(struct model *model, char *dirname, char *mtllib)
{
	char filename[1024];
	strcpy(filename, dirname);
	strcat(filename, "/");
	strcat(filename, mtllib);
}

static void splitfv(char *buf, int *vp, int *vtp, int *vnp)
{
	char tmp[200], *p, *v, *vt, *vn;
	strcpy(tmp, buf);
	p = buf;
	v = strsep(&p, "/");
	vt = strsep(&p, "/");
	vn = strsep(&p, "/");
	if (v) *vp = atoi(v) - 1;
	if (vt) *vtp = atoi(v) - 1;
	if (vn) *vnp = atoi(v) - 1;
}

struct model *load_model_obj(char *filename)
{
	char dirname[1024];
	char line[200];
	int fv[20], fvt[20], fvn[20];
	struct model *model;
	char *p, *s;
	int i, n;
	FILE *fp;

	strcpy(dirname, filename);
	p = strrchr(dirname, '/');
	if (!p) p = strrchr(dirname, '\\');
	if (p) *p = 0;
	else strcpy(dirname, ".");

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "error: cannot load model '%s'\n", filename);
		return NULL;
	}

	model = malloc(sizeof(struct model));

	while (1) {
		if (!fgets(line, sizeof line, fp))
			break;

		p = line;
		if (!p[0] || p[0] == '#')
			continue;

		s = strsep(&p, " ");
		if (!strcmp(s, "v")) {
			char *x = strsep(&p, " ");
			char *y = strsep(&p, " ");
			char *z = strsep(&p, " ");
			addv(atof(x), atof(y), atof(z));
		} else if (!strcmp(s, "vt")) {
			char *u = strsep(&p, " ");
			char *v = strsep(&p, " ");
			addvt(atof(u), atof(v));
		} else if (!strcmp(s, "vn")) {
			char *x = strsep(&p, " ");
			char *y = strsep(&p, " ");
			char *z = strsep(&p, " ");
			addvn(atof(x), atof(y), atof(z));
		} else if (!strcmp(s, "f")) {
			n = 0;
			s = strsep(&p, " ");
			while (s) {
				splitfv(s, fv+n, fvt+n, fvn+n);
				s = strsep(&p, " ");
			}
			for (i = 1; i < n - 1; i++) {
				addf(fv[0], fvt[0], fvn[0],
					fv[i], fvt[i], fvn[i],
					fv[i+1], fvt[i+1], fvn[i+1]);
			}
		} else if (!strcmp(s, "mtllib")) {
			s = strsep(&p, " ");
			load_material(model, dirname, s);
		} else if (!strcmp(s, "usemtl")) {
			s = strsep(&p, " ");
		}
	}

	fclose(fp);

	return model;
}
