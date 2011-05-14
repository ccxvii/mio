#include "mio.h"

#include <unistd.h> /* for access() */

static void die(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

/* Quake normal vector table */
static const float quake_normals[162][3] = {
	{ -0.525731f,  0.000000f,  0.850651f },
	{ -0.442863f,  0.238856f,  0.864188f },
	{ -0.295242f,  0.000000f,  0.955423f },
	{ -0.309017f,  0.500000f,  0.809017f },
	{ -0.162460f,  0.262866f,  0.951056f },
	{  0.000000f,  0.000000f,  1.000000f },
	{  0.000000f,  0.850651f,  0.525731f },
	{ -0.147621f,  0.716567f,  0.681718f },
	{  0.147621f,  0.716567f,  0.681718f },
	{  0.000000f,  0.525731f,  0.850651f },
	{  0.309017f,  0.500000f,  0.809017f },
	{  0.525731f,  0.000000f,  0.850651f },
	{  0.295242f,  0.000000f,  0.955423f },
	{  0.442863f,  0.238856f,  0.864188f },
	{  0.162460f,  0.262866f,  0.951056f },
	{ -0.681718f,  0.147621f,  0.716567f },
	{ -0.809017f,  0.309017f,  0.500000f },
	{ -0.587785f,  0.425325f,  0.688191f },
	{ -0.850651f,  0.525731f,  0.000000f },
	{ -0.864188f,  0.442863f,  0.238856f },
	{ -0.716567f,  0.681718f,  0.147621f },
	{ -0.688191f,  0.587785f,  0.425325f },
	{ -0.500000f,  0.809017f,  0.309017f },
	{ -0.238856f,  0.864188f,  0.442863f },
	{ -0.425325f,  0.688191f,  0.587785f },
	{ -0.716567f,  0.681718f, -0.147621f },
	{ -0.500000f,  0.809017f, -0.309017f },
	{ -0.525731f,  0.850651f,  0.000000f },
	{  0.000000f,  0.850651f, -0.525731f },
	{ -0.238856f,  0.864188f, -0.442863f },
	{  0.000000f,  0.955423f, -0.295242f },
	{ -0.262866f,  0.951056f, -0.162460f },
	{  0.000000f,  1.000000f,  0.000000f },
	{  0.000000f,  0.955423f,  0.295242f },
	{ -0.262866f,  0.951056f,  0.162460f },
	{  0.238856f,  0.864188f,  0.442863f },
	{  0.262866f,  0.951056f,  0.162460f },
	{  0.500000f,  0.809017f,  0.309017f },
	{  0.238856f,  0.864188f, -0.442863f },
	{  0.262866f,  0.951056f, -0.162460f },
	{  0.500000f,  0.809017f, -0.309017f },
	{  0.850651f,  0.525731f,  0.000000f },
	{  0.716567f,  0.681718f,  0.147621f },
	{  0.716567f,  0.681718f, -0.147621f },
	{  0.525731f,  0.850651f,  0.000000f },
	{  0.425325f,  0.688191f,  0.587785f },
	{  0.864188f,  0.442863f,  0.238856f },
	{  0.688191f,  0.587785f,  0.425325f },
	{  0.809017f,  0.309017f,  0.500000f },
	{  0.681718f,  0.147621f,  0.716567f },
	{  0.587785f,  0.425325f,  0.688191f },
	{  0.955423f,  0.295242f,  0.000000f },
	{  1.000000f,  0.000000f,  0.000000f },
	{  0.951056f,  0.162460f,  0.262866f },
	{  0.850651f, -0.525731f,  0.000000f },
	{  0.955423f, -0.295242f,  0.000000f },
	{  0.864188f, -0.442863f,  0.238856f },
	{  0.951056f, -0.162460f,  0.262866f },
	{  0.809017f, -0.309017f,  0.500000f },
	{  0.681718f, -0.147621f,  0.716567f },
	{  0.850651f,  0.000000f,  0.525731f },
	{  0.864188f,  0.442863f, -0.238856f },
	{  0.809017f,  0.309017f, -0.500000f },
	{  0.951056f,  0.162460f, -0.262866f },
	{  0.525731f,  0.000000f, -0.850651f },
	{  0.681718f,  0.147621f, -0.716567f },
	{  0.681718f, -0.147621f, -0.716567f },
	{  0.850651f,  0.000000f, -0.525731f },
	{  0.809017f, -0.309017f, -0.500000f },
	{  0.864188f, -0.442863f, -0.238856f },
	{  0.951056f, -0.162460f, -0.262866f },
	{  0.147621f,  0.716567f, -0.681718f },
	{  0.309017f,  0.500000f, -0.809017f },
	{  0.425325f,  0.688191f, -0.587785f },
	{  0.442863f,  0.238856f, -0.864188f },
	{  0.587785f,  0.425325f, -0.688191f },
	{  0.688191f,  0.587785f, -0.425325f },
	{ -0.147621f,  0.716567f, -0.681718f },
	{ -0.309017f,  0.500000f, -0.809017f },
	{  0.000000f,  0.525731f, -0.850651f },
	{ -0.525731f,  0.000000f, -0.850651f },
	{ -0.442863f,  0.238856f, -0.864188f },
	{ -0.295242f,  0.000000f, -0.955423f },
	{ -0.162460f,  0.262866f, -0.951056f },
	{  0.000000f,  0.000000f, -1.000000f },
	{  0.295242f,  0.000000f, -0.955423f },
	{  0.162460f,  0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f },
	{ -0.162460f, -0.262866f, -0.951056f },
	{  0.000000f, -0.850651f, -0.525731f },
	{ -0.147621f, -0.716567f, -0.681718f },
	{  0.147621f, -0.716567f, -0.681718f },
	{  0.000000f, -0.525731f, -0.850651f },
	{  0.309017f, -0.500000f, -0.809017f },
	{  0.442863f, -0.238856f, -0.864188f },
	{  0.162460f, -0.262866f, -0.951056f },
	{  0.238856f, -0.864188f, -0.442863f },
	{  0.500000f, -0.809017f, -0.309017f },
	{  0.425325f, -0.688191f, -0.587785f },
	{  0.716567f, -0.681718f, -0.147621f },
	{  0.688191f, -0.587785f, -0.425325f },
	{  0.587785f, -0.425325f, -0.688191f },
	{  0.000000f, -0.955423f, -0.295242f },
	{  0.000000f, -1.000000f,  0.000000f },
	{  0.262866f, -0.951056f, -0.162460f },
	{  0.000000f, -0.850651f,  0.525731f },
	{  0.000000f, -0.955423f,  0.295242f },
	{  0.238856f, -0.864188f,  0.442863f },
	{  0.262866f, -0.951056f,  0.162460f },
	{  0.500000f, -0.809017f,  0.309017f },
	{  0.716567f, -0.681718f,  0.147621f },
	{  0.525731f, -0.850651f,  0.000000f },
	{ -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f },
	{ -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f,  0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f,  0.147621f },
	{ -0.525731f, -0.850651f,  0.000000f },
	{ -0.500000f, -0.809017f,  0.309017f },
	{ -0.238856f, -0.864188f,  0.442863f },
	{ -0.262866f, -0.951056f,  0.162460f },
	{ -0.864188f, -0.442863f,  0.238856f },
	{ -0.809017f, -0.309017f,  0.500000f },
	{ -0.688191f, -0.587785f,  0.425325f },
	{ -0.681718f, -0.147621f,  0.716567f },
	{ -0.442863f, -0.238856f,  0.864188f },
	{ -0.587785f, -0.425325f,  0.688191f },
	{ -0.309017f, -0.500000f,  0.809017f },
	{ -0.147621f, -0.716567f,  0.681718f },
	{ -0.425325f, -0.688191f,  0.587785f },
	{ -0.162460f, -0.262866f,  0.951056f },
	{  0.442863f, -0.238856f,  0.864188f },
	{  0.162460f, -0.262866f,  0.951056f },
	{  0.309017f, -0.500000f,  0.809017f },
	{  0.147621f, -0.716567f,  0.681718f },
	{  0.000000f, -0.525731f,  0.850651f },
	{  0.425325f, -0.688191f,  0.587785f },
	{  0.587785f, -0.425325f,  0.688191f },
	{  0.688191f, -0.587785f,  0.425325f },
	{ -0.955423f,  0.295242f,  0.000000f },
	{ -0.951056f,  0.162460f,  0.262866f },
	{ -1.000000f,  0.000000f,  0.000000f },
	{ -0.850651f,  0.000000f,  0.525731f },
	{ -0.955423f, -0.295242f,  0.000000f },
	{ -0.951056f, -0.162460f,  0.262866f },
	{ -0.864188f,  0.442863f, -0.238856f },
	{ -0.951056f,  0.162460f, -0.262866f },
	{ -0.809017f,  0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f },
	{ -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f,  0.147621f, -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f },
	{ -0.850651f,  0.000000f, -0.525731f },
	{ -0.688191f,  0.587785f, -0.425325f },
	{ -0.587785f,  0.425325f, -0.688191f },
	{ -0.425325f,  0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f },
	{ -0.688191f, -0.587785f, -0.425325f }
};

struct md2_texcoord
{
	short s;
	short t;
};

struct md2_triangle
{
	unsigned short vertex[3];
	unsigned short st[3];
};

struct md2_vertex
{
	unsigned char v[3];
	unsigned char normal;
};

struct md2_frame
{
	float scale[3];
	float translate[3];
	char name[16];
	struct md2_vertex *verts;
};

union md2_cmd
{
	int i;
	float f;
};

struct md2_model
{
	int numskins;
	int skinwidth;
	int skinheight;

	int numverts;
	int numst;
	int numtris;
	int numcmds;
	int numframes;

	unsigned int *skintextures;
	struct md2_texcoord *texcoords;
	struct md2_triangle *triangles;
	struct md2_frame *frames;
	union md2_cmd *cmds;
};

static inline int getshort(FILE *fp)
{
	int a = getc(fp);
	int b = getc(fp);
	return (b << 8) | a;
}

static inline int getint(FILE *fp)
{
	int a = getc(fp);
	int b = getc(fp);
	int c = getc(fp);
	int d = getc(fp);
	return (d << 24) | (c << 16) | (b << 8) | a;
}

static inline float getfloat(FILE *fp)
{
	union { int i; float f; } u;
	u.i = getint(fp);
	return u.f;
}

static inline void getvec3(float *v, FILE *fp)
{
	v[0] = getfloat(fp);
	v[1] = getfloat(fp);
	v[2] = getfloat(fp);
}

static int find_skin(char *filename, char *modelname, char *skinname)
{
	char directory[64];
	char modelbase[64];
	char skinbase[64];
	char *p;

	if (access(skinname, F_OK) == 0)
	{
		strcpy(filename, skinname);
		return 1;
	}

	strcpy(directory, modelname);
	p = strrchr(directory, '/');
	if (p) p[0] = 0; else directory[0] = 0;

	p = strrchr(modelname, '/');
	if (p) p++; else p = modelname;
	strcpy(modelbase, p);
	p = strrchr(modelbase, '.');
	if (p) *p++ = 0;

	p = strrchr(skinname, '/');
	if (p) p++; else p = skinname;
	strcpy(skinbase, p);
	p = strrchr(skinbase, '.');
	if (p) *p++ = 0;

	sprintf(filename, "%s/%s.png", directory, skinbase);
	if (access(filename, F_OK) == 0)
		return 1;

	sprintf(filename, "%s/%s.png", directory, modelbase);
	if (access(filename, F_OK) == 0)
		return 1;

	sprintf(filename, "%s/skin.png", directory);
	if (access(filename, F_OK) == 0)
		return 1;

	return 0;
}

struct md2_model * md2_load_model(char *modelname)
{
	struct md2_model *self;
	char ident[4];
	int version;
	int i, w, h, n;
	FILE *fp;

	fp = fopen(modelname, "rb");
	if (!fp)
		die("cannot open model file");

	printf("loading md2 model: %s\n", modelname);

	n = fread(ident, 1, 4, fp);
	if (n != 4 || memcmp(ident, "IDP2", 4))
		die("wrong magic number: not a quake md2 file");

	version = getint(fp);
	if (version != 8)
		die("wrong version number");

	self = malloc(sizeof(struct md2_model));
	if (!self)
		die("cannot allocate model structure");

	self->skinwidth = getint(fp);
	self->skinheight = getint(fp);

	(void) getint(fp); /* framesize */

	self->numskins = getint(fp);
	self->numverts = getint(fp);
	self->numst = getint(fp);
	self->numtris = getint(fp);
	self->numcmds = getint(fp);
	self->numframes = getint(fp);

	int offset_skins = getint(fp);
	int offset_st = getint(fp);
	int offset_tris = getint(fp);
	int offset_frames = getint(fp);
	int offset_cmds = getint(fp);
	int offset_end = getint(fp);

	self->skintextures = malloc(sizeof(int) * self->numskins);
	self->texcoords = malloc(sizeof(struct md2_texcoord) * self->numst);
	self->triangles = malloc(sizeof(struct md2_triangle) * self->numtris);
	self->frames = malloc(sizeof(struct md2_frame) * self->numframes);
	for (i = 0; i < self->numframes; i++)
		self->frames[i].verts = malloc(sizeof(struct md2_vertex) * self->numverts);
	self->cmds = malloc(sizeof(union md2_cmd) * self->numcmds);

	fseek(fp, offset_skins, 0);
	for (i = 0; i < self->numskins; i++)
	{
		char filename[64];
		char skinname[64];
		n = fread(skinname, 1, 64, fp);
		if (n == 64 && find_skin(filename, modelname, skinname))
		{
			self->skintextures[i] = load_texture(filename, &w, &h, 0, 0);
			if (w != self->skinwidth || h != self->skinheight)
				die("skin image does not match dimensions given in model file");
		}
		else
		{
			printf("cannot find skin file: %s\n", skinname);
			self->skintextures[i] = 0;
		}
	}

	fseek(fp, offset_st, 0);
	for (i = 0; i < self->numst; i++)
	{
		self->texcoords[i].s = getshort(fp);
		self->texcoords[i].t = getshort(fp);
	}

	fseek(fp, offset_tris, 0);
	for (i = 0; i < self->numtris; i++)
	{
		self->triangles[i].vertex[0] = getshort(fp);
		self->triangles[i].vertex[1] = getshort(fp);
		self->triangles[i].vertex[2] = getshort(fp);
		self->triangles[i].st[0] = getshort(fp);
		self->triangles[i].st[1] = getshort(fp);
		self->triangles[i].st[2] = getshort(fp);
	}

	fseek(fp, offset_frames, 0);
	for (i = 0; i < self->numframes; i++)
	{
		getvec3(self->frames[i].scale, fp);
		getvec3(self->frames[i].translate, fp);
		n = fread(self->frames[i].name, 1, 16, fp);
		n = fread(self->frames[i].verts, 4, self->numverts, fp);
	}

	fseek(fp, offset_cmds, 0);
	for (i = 0; i < self->numcmds; i++)
	{
		self->cmds[i].i = getint(fp);
	}

	fclose(fp);

	return self;
}

void md2_free_model(struct md2_model *self)
{
	int i;
	glDeleteTextures(self->numskins, self->skintextures);
	for (i = 0; i < self->numframes; i++)
		free(self->frames[i].verts);
	free(self->skintextures);
	free(self->texcoords);
	free(self->triangles);
	free(self->frames);
	free(self->cmds);
	free(self);
}

int md2_get_frame_count(struct md2_model *self)
{
	return self->numframes;
}

char *md2_get_frame_name(struct md2_model *self, int idx)
{
	if (idx < 0 || idx >= self->numframes)
		return NULL;
	return self->frames[idx].name;
}

static void md2_draw_frame_tri(struct md2_model *self, int frame)
{
	struct md2_frame *pframe = self->frames + frame;
	struct md2_vertex *pvert;
	int vertex, i, k;
	float s, t;
	float v[3];

	glBegin(GL_TRIANGLES);

	for (i = 0; i < self->numtris; i++)
	{
		for (k = 0; k < 3; k++)
		{
			vertex = self->triangles[i].vertex[k];
			pvert = pframe->verts + vertex;

			s = (self->texcoords[self->triangles[i].st[k]].s + 0.5) / self->skinwidth;
			t = (self->texcoords[self->triangles[i].st[k]].t + 0.5) / self->skinheight;
			glTexCoord2f(s, t);

			glNormal3fv(quake_normals[pvert->normal]);

			v[0] = pframe->scale[0] * pvert->v[0] + pframe->translate[0];
			v[1] = pframe->scale[1] * pvert->v[1] + pframe->translate[1];
			v[2] = pframe->scale[2] * pvert->v[2] + pframe->translate[2];
			glVertex3fv(v);
		}
	}

	glEnd();
}

static void md2_draw_frame_lerp_tri(struct md2_model *self, int frame0, int frame1, float lerp)
{
	struct md2_frame *pframe0 = self->frames + frame0;
	struct md2_frame *pframe1 = self->frames + frame1;
	struct md2_vertex *pvert0;
	struct md2_vertex *pvert1;
	int vertex, i, k;
	float s, t;
	const float *na, *nb;
	float va[3], vb[3], v[3], n[3];

	glBegin(GL_TRIANGLES);

	for (i = 0; i < self->numtris; i++)
	{
		for (k = 0; k < 3; k++)
		{
			vertex = self->triangles[i].vertex[k];
			pvert0 = pframe0->verts + vertex;
			pvert1 = pframe1->verts + vertex;

			s = (self->texcoords[self->triangles[i].st[k]].s + 0.5) / self->skinwidth;
			t = (self->texcoords[self->triangles[i].st[k]].t + 0.5) / self->skinheight;
			glTexCoord2f(s, t);

			na = quake_normals[pvert0->normal];
			nb = quake_normals[pvert1->normal];
			n[0] = na[0] + (nb[0] - na[0]) * lerp;
			n[1] = na[1] + (nb[1] - na[1]) * lerp;
			n[2] = na[2] + (nb[2] - na[2]) * lerp;
			glNormal3fv(n);

			va[0] = pframe0->scale[0] * pvert0->v[0] + pframe0->translate[0];
			va[1] = pframe0->scale[1] * pvert0->v[1] + pframe0->translate[1];
			va[2] = pframe0->scale[2] * pvert0->v[2] + pframe0->translate[2];
			vb[0] = pframe1->scale[0] * pvert1->v[0] + pframe1->translate[0];
			vb[1] = pframe1->scale[1] * pvert1->v[1] + pframe1->translate[1];
			vb[2] = pframe1->scale[2] * pvert1->v[2] + pframe1->translate[2];
			v[0] = va[0] + (vb[0] - va[0]) * lerp;
			v[1] = va[1] + (vb[1] - va[1]) * lerp;
			v[2] = va[2] + (vb[2] - va[2]) * lerp;
			glVertex3fv(v);
		}
	}

	glEnd();
}

static void md2_draw_frame_cmd(struct md2_model *self, int frame)
{
	struct md2_frame *pframe = self->frames + frame;
	union md2_cmd *pcmd = self->cmds;
	struct md2_vertex *pvert;
	float v[3];
	int count;

	count = (pcmd++)->i;
	while (count != 0)
	{
		if (count < 0)
		{
			glBegin(GL_TRIANGLE_FAN);
			count = -count;
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);
		}

		while (count--)
		{
			float s = (pcmd++)->f;
			float t = (pcmd++)->f;
			int vertex = (pcmd++)->i;
			pvert = pframe->verts + vertex;

			glTexCoord2f(s, t);

			glNormal3fv(quake_normals[pvert->normal]);

			v[0] = pframe->scale[0] * pvert->v[0] + pframe->translate[0];
			v[1] = pframe->scale[1] * pvert->v[1] + pframe->translate[1];
			v[2] = pframe->scale[2] * pvert->v[2] + pframe->translate[2];
			glVertex3fv(v);
		}

		glEnd();

		count = (pcmd++)->i;
	}
}

static void md2_draw_frame_lerp_cmd(struct md2_model *self, int frame0, int frame1, float lerp)
{
	struct md2_frame *pframe0 = self->frames + frame0;
	struct md2_frame *pframe1 = self->frames + frame1;
	union md2_cmd *pcmd = self->cmds;
	struct md2_vertex *pvert0;
	struct md2_vertex *pvert1;
	const float *na, *nb;
	float va[3], vb[3], v[3], n[3];
	int count;

	count = (pcmd++)->i;
	while (count != 0)
	{
		if (count < 0)
		{
			glBegin(GL_TRIANGLE_FAN);
			count = -count;
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);
		}

		while (count--)
		{
			float s = (pcmd++)->f;
			float t = (pcmd++)->f;
			int vertex = (pcmd++)->i;
			pvert0 = pframe0->verts + vertex;
			pvert1 = pframe1->verts + vertex;

			glTexCoord2f(s, t);

			na = quake_normals[pvert0->normal];
			nb = quake_normals[pvert1->normal];
			n[0] = na[0] + (nb[0] - na[0]) * lerp;
			n[1] = na[1] + (nb[1] - na[1]) * lerp;
			n[2] = na[2] + (nb[2] - na[2]) * lerp;
			glNormal3fv(n);

			va[0] = pframe0->scale[0] * pvert0->v[0] + pframe0->translate[0];
			va[1] = pframe0->scale[1] * pvert0->v[1] + pframe0->translate[1];
			va[2] = pframe0->scale[2] * pvert0->v[2] + pframe0->translate[2];
			vb[0] = pframe1->scale[0] * pvert1->v[0] + pframe1->translate[0];
			vb[1] = pframe1->scale[1] * pvert1->v[1] + pframe1->translate[1];
			vb[2] = pframe1->scale[2] * pvert1->v[2] + pframe1->translate[2];
			v[0] = va[0] + (vb[0] - va[0]) * lerp;
			v[1] = va[1] + (vb[1] - va[1]) * lerp;
			v[2] = va[2] + (vb[2] - va[2]) * lerp;
			glVertex3fv(v);
		}

		glEnd();

		count = (pcmd++)->i;
	}
}

void md2_draw_frame(struct md2_model *self, int skin, int frame)
{
	if (skin < 0 || frame < 0)
		return;
	if (skin >= self->numskins || frame >= self->numframes)
		return;
	glBindTexture(GL_TEXTURE_2D, self->skintextures[skin]);
	if (self->numcmds > 0)
		md2_draw_frame_cmd(self, frame);
	else
		md2_draw_frame_tri(self, frame);
}

void md2_draw_frame_lerp(struct md2_model *self, int skin, int idx0, int idx1, float lerp)
{
	if (skin < 0 || idx0 < 0 || idx1 < 0)
		return;
	if (skin >= self->numskins || idx0 >= self->numframes || idx1 >= self->numframes)
		return;
	if (lerp < 0.0)
		lerp = 0.0;
	if (lerp > 1.0)
		lerp = 1.0;
	glBindTexture(GL_TEXTURE_2D, self->skintextures[skin]);
	if (self->numcmds > 0)
		md2_draw_frame_lerp_cmd(self, idx0, idx1, lerp);
	else
		md2_draw_frame_lerp_tri(self, idx0, idx1, lerp);
}
