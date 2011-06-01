#include "mio.h"

#define noFLIP
#define NAMELEN 80

struct material {
	char name[NAMELEN];
	int texture;
};

struct pose {
	float translate[3];
	float rotate[4];
	float scale[3];
};

struct bone {
	char name[NAMELEN];
	int parent;
	struct pose bind_pose;
	float bind_matrix[4][4];
	float inv_bind_matrix[4][4];
};

struct bounds {
	float min[3], max[3];
	float xyradius, radius;
};

struct anim {
	char name[NAMELEN];
	int first, count;
	float rate;
	int loop;
};

struct mesh {
	char name[NAMELEN];
	int material; // struct material *material;
	int first, count;
};

struct model {
	char dir[NAMELEN];
	int num_verts, num_tris, num_meshes, num_bones, num_frames, num_anims;
	float *pos, *norm, *texcoord;
	unsigned char *blend_index, *blend_weight, *color;
	int *tris;
	struct mesh *meshes;
	struct bone *bones;
	struct bounds *bounds; // bound for each frame
	struct pose **poses; // poses for each frame
	struct anim *anims;

	float *outpos, *outnorm;
	float (*outbone)[4][4];
	float (*outskin)[4][4];
	struct pose *outpose;
};

#define IQM_MAGIC "INTERQUAKEMODEL\0"
#define IQM_VERSION 2

enum {
	IQM_POSITION = 0,
	IQM_TEXCOORD = 1,
	IQM_NORMAL = 2,
	IQM_TANGENT = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR = 6,
	IQM_CUSTOM = 0x10
};

enum {
	IQM_BYTE = 0,
	IQM_UBYTE = 1,
	IQM_SHORT = 2,
	IQM_USHORT = 3,
	IQM_INT = 4,
	IQM_UINT = 5,
	IQM_HALF = 6,
	IQM_FLOAT = 7,
	IQM_DOUBLE = 8,
};

static inline void restorew(float q[4])
{
	q[3] = -sqrt(max(1 - q[0]*q[0] - q[1]*q[1] - q[2]*q[2], 0));
	if (q[3] > -0.01) printf("warning: q->w near zero (%g %g %g : %g)\n", q[0],q[1],q[2],q[3]);
}

static inline int read32(unsigned char *data)
{
	return data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
}

static inline unsigned short read16(unsigned char *data)
{
	return data[0] | data[1] << 8;
}

static inline float readfloat(unsigned char *data)
{
	union { float f; int i; } u;
	u.i = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	return u.f;
}

static void float44_from_quat_vec(float m[4][4], float *q, float *v)
{
	float x2 = q[0] + q[0];
	float y2 = q[1] + q[1];
	float z2 = q[2] + q[2];
	{
		float xx2 = q[0] * x2;
		float yy2 = q[1] * y2;
		float zz2 = q[2] * z2;
		m[0][0] = 1 - yy2 - zz2;
		m[1][1] = 1 - xx2 - zz2;
		m[2][2] = 1 - xx2 - yy2;
	}
	{
		float yz2 = q[1] * z2;
		float wx2 = q[3] * x2;
		m[2][1] = yz2 + wx2;
		m[1][2] = yz2 - wx2;
	}
	{
		float xy2 = q[0] * y2;
		float wz2 = q[3] * z2;
		m[1][0] = xy2 + wz2;
		m[0][1] = xy2 - wz2;
	}
	{
		float xz2 = q[0] * z2;
		float wy2 = q[3] * y2;
		m[0][2] = xz2 + wy2;
		m[2][0] = xz2 - wy2;
	}

	m[0][3] = v[0];
	m[1][3] = v[1];
	m[2][3] = v[2];

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
}

static void float44_vec_mul(float *out, float m[4][4], float *v)
{
	out[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3];
	out[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3];
	out[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3];
}

static void float44_vec_mul_n(float *out, float m[4][4], float *v)
{
	out[0] = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
	out[1] = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
	out[2] = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];
}

static void float16_inverse(float out[16], const float m[16])
{
	float inv[16], det;
	int i;

	inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] +
		m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] -
		m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] +
		m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
	inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] -
		m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
	inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] -
		m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] +
		m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] -
		m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
	inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] +
		m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
	inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] +
		m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
	inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] -
		m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
	inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] +
		m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
	inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] -
		m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
	inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] -
		m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
	inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] +
		m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
	inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] -
		m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
	inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] +
		m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	assert (det != 0);
	det = 1.0 / det;
	for (i = 0; i < 16; i++)
		out[i] = inv[i] * det;
}

static void float44_inverse(float out[4][4], float mat[4][4])
{
	float16_inverse((float*)out, (float*)mat);
}

static void float44_mul(float out[4][4], float mat1[4][4], float mat2[4][4])
{
	float temp1[4][4], temp2[4][4];
	int i, j;
	if (mat1 == out) { memcpy(temp1, mat1, sizeof(temp1)); mat1 = temp1; }
	if (mat2 == out) { memcpy(temp2, mat2, sizeof(temp2)); mat2 = temp2; }
	for (j = 0; j < 4; ++j)
		for (i = 0; i < 4; ++i)
			out[j][i] =
				mat1[0][i] * mat2[j][0] +
				mat1[1][i] * mat2[j][1] +
				mat1[2][i] * mat2[j][2] +
				mat1[3][i] * mat2[j][3];
}

static void float44_print(float m[4][4])
{
	int i, j;
	for (j = 0; j < 4; ++j) {
		printf("\t");
		for (i = 0; i < 4; i++)
			printf("%+f%c", m[j][i], i==3?'\n':' ');
	}
}

static void print_pose(struct pose *pose)
{
	printf("pq %f %f %f\t%f %f %f %f\t%g %g %g\n",
		pose->translate[0], pose->translate[1], pose->translate[2],
		pose->rotate[0], pose->rotate[1], pose->rotate[2], pose->rotate[3],
		pose->scale[0], pose->scale[1], pose->scale[2]);
}

static float *
load_float_array(unsigned char *data, int size, int count)
{
	float *a = malloc(size * count * sizeof(float));
	int i, n = count * size;
	for (i = 0; i < n; i++)
		a[i] = readfloat(data + (i << 2));
	return a;
}

static unsigned char *
load_byte_array(unsigned char *data, int size, int count)
{
	unsigned char *a = malloc(size * count);
	memcpy(a, data, size * count);
	return a;
}

static void
load_vertex_arrays(struct model *model, unsigned char *data, int ofs_va, int num_va, int num_v)
{
	int i, k;
	model->num_verts = num_v;
	for (i = 0; i < num_va; i++) {
		int type = read32(data + ofs_va + 0);
		int flags = read32(data + ofs_va + 4);
		int format = read32(data + ofs_va + 8);
		int size = read32(data + ofs_va + 12);
		int offset = read32(data + ofs_va + 16);
		switch (type) {
		case IQM_POSITION:
			assert(format == IQM_FLOAT && size == 3);
			model->pos = load_float_array(data + offset, size, num_v);
			break;
		case IQM_TEXCOORD:
			assert(format == IQM_FLOAT && size == 2);
			model->texcoord = load_float_array(data + offset, size, num_v);
			break;
		case IQM_NORMAL:
			assert(format == IQM_FLOAT && size == 3);
			model->norm = load_float_array(data + offset, size, num_v);
#ifdef FLIP
			for (k = 0; k < num_v * 3; k++)
				model->norm[k] = -model->norm[k]; // IQM has reversed winding
#endif
			break;
		case IQM_BLENDINDEXES:
			assert(format == IQM_UBYTE && size == 4);
			model->blend_index = load_byte_array(data + offset, size, num_v);
			break;
		case IQM_BLENDWEIGHTS:
			assert(format == IQM_UBYTE && size == 4);
			model->blend_weight = load_byte_array(data + offset, size, num_v);
			break;
		case IQM_COLOR:
			assert(format == IQM_UBYTE && size == 4);
			model->color = load_byte_array(data + offset, size, num_v);
			break;
		}
		ofs_va += 20;
	}
}

static void
load_triangles(struct model *model, unsigned char *data, int ofs_tris, int num_tris)
{
	unsigned char *p = data + ofs_tris;
	int *t;
	model->num_tris = num_tris;
	model->tris = t = malloc(num_tris * 3 * sizeof(int));
	while (num_tris--) {
#ifdef FLIP
		t[0] = read32(p + 0);
		t[2] = read32(p + 4); // IQM has reversed winding
		t[1] = read32(p + 8);
#else
		t[0] = read32(p + 0);
		t[1] = read32(p + 4);
		t[2] = read32(p + 8);
#endif
		t += 3;
		p += 12;
	}
}

static int
load_material(struct model *model, char *name)
{
	char buf[256], *p;
	strlcpy(buf, model->dir, sizeof buf);
	strlcat(buf, name, sizeof buf);
	p = strrchr(buf, ',');
	if (p) strlcpy(p, ".png", sizeof buf - (p-buf));
	return load_texture(buf, 0, 0, 0, 0);
}

static void
load_meshes(struct model *model, unsigned char *data, int ofs_text, int ofs_meshes, int num_meshes)
{
	int i;
	model->num_meshes = num_meshes;
	model->meshes = malloc(num_meshes * sizeof(struct mesh));
	for (i = 0; i < num_meshes; i++) {
		char *name = data + ofs_text + read32(data + ofs_meshes + 0);
		char *material = data + ofs_text + read32(data + ofs_meshes + 4);
		strlcpy(model->meshes[i].name, name, NAMELEN);
		model->meshes[i].material = load_material(model, material);
		model->meshes[i].first = read32(data + ofs_meshes + 16);
		model->meshes[i].count = read32(data + ofs_meshes + 20);
		ofs_meshes += 24;
	}
}

static void read_pose(struct pose *pose, unsigned char *data)
{
	pose->translate[0] = readfloat(data + 0);
	pose->translate[1] = readfloat(data + 4);
	pose->translate[2] = readfloat(data + 8);
	pose->rotate[0] = readfloat(data + 12);
	pose->rotate[1] = readfloat(data + 16);
	pose->rotate[2] = readfloat(data + 20);
	pose->rotate[3] = readfloat(data + 24);
	pose->scale[0] = readfloat(data + 28);
	pose->scale[1] = readfloat(data + 32);
	pose->scale[2] = readfloat(data + 36);
	float *q = pose->rotate;
	float mag = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
	if (fabs(mag-1) > 0.001)
		printf("non-unit quat in bone! (len=%g)\n", mag);
}

static void
load_bones(struct model *model, unsigned char *data, int ofs_text, int ofs_bones, int ofs_poses, int num_bones)
{
	int i;
	model->num_bones = num_bones;
	model->bones = malloc(num_bones * sizeof(struct bone));
	for (i = 0; i < num_bones; i++) {
		struct bone *bone = model->bones + i;
		char *name = data + ofs_text + read32(data + ofs_bones + 0);
		bone->parent = read32(data + ofs_bones + 4);
		strlcpy(bone->name, name, NAMELEN);
		read_pose(&bone->bind_pose, data + ofs_bones + 8);
		float44_from_quat_vec(bone->bind_matrix, bone->bind_pose.rotate, bone->bind_pose.translate);
		if (bone->parent >= 0) {
			struct bone *parent = model->bones + bone->parent;
			float44_mul(bone->bind_matrix, bone->bind_matrix, parent->bind_matrix);
		}
		float44_inverse(bone->inv_bind_matrix, bone->bind_matrix); // must be orthogonal
		ofs_bones += 48;
	}
}

static void
load_anims(struct model *model, unsigned char *data, int ofs_text, int ofs_anims, int num_anims)
{
	int i;
	model->num_anims = num_anims;
	model->anims = malloc(num_anims * sizeof(struct anim));
	for (i = 0; i < num_anims; i++) {
		struct anim *anim = model->anims + i;
		char *name = data + ofs_text + read32(data + ofs_anims + 0);
		strlcpy(anim->name, name, NAMELEN);
		anim->first = read32(data + ofs_anims + 4);
		anim->count = read32(data + ofs_anims + 8);
		anim->rate = readfloat(data + ofs_anims + 12);
		anim->loop = read32(data + ofs_anims + 16);
		ofs_anims += 20;
	}
}

struct chan {
	int mask;
	float offset[10];
	float scale[10];
};

static void
load_frames(struct model *model, unsigned char *data,
	int ofs_poses, int ofs_frames, int ofs_bounds, int num_frames, int num_framechannels)
{
	int i, k, n, num_bones = model->num_bones;
	struct chan *chans = malloc(num_bones * sizeof(struct chan));
	unsigned char *p = data + ofs_frames;
	model->num_frames = num_frames;
	model->bounds = malloc(num_frames * num_bones * sizeof(struct bounds));
	model->poses = malloc(num_frames * sizeof(struct pose*));

	for (k = 0; k < num_bones; k++) {
		struct chan *chan = chans + k;
		chan->mask = read32(data + ofs_poses + 4);
		for (n = 0; n < 10; n++) {
			chan->offset[n] = readfloat(data + ofs_poses + 8 + n * 4);
			chan->scale[n] = readfloat(data + ofs_poses + 8 + 10 * 4 + n * 4);
		}
		ofs_poses += 22 * 4;
	}

	for (i = 0; i < num_frames; i++) {
		model->poses[i] = malloc(num_bones * sizeof(struct pose));
		for (k = 0; k < num_bones; k++) {
			struct chan *chan = chans + k;
			struct pose *pose = model->poses[i] + k;
			for (n = 0; n < 3; n++) {
				pose->translate[n] = chan->offset[n];
				pose->rotate[n] = chan->offset[3+n];
				pose->scale[n] = chan->offset[7+n];
			}
			pose->rotate[3] = chan->offset[6];
			if (chan->mask & 0x01) { pose->translate[0] += read16(p) * chan->scale[0]; p += 2; }
			if (chan->mask & 0x02) { pose->translate[1] += read16(p) * chan->scale[1]; p += 2; }
			if (chan->mask & 0x04) { pose->translate[2] += read16(p) * chan->scale[2]; p += 2; }
			if (chan->mask & 0x08) { pose->rotate[0] += read16(p) * chan->scale[3]; p += 2; }
			if (chan->mask & 0x10) { pose->rotate[1] += read16(p) * chan->scale[4]; p += 2; }
			if (chan->mask & 0x20) { pose->rotate[2] += read16(p) * chan->scale[5]; p += 2; }
			if (chan->mask & 0x40) { pose->rotate[3] += read16(p) * chan->scale[6]; p += 2; }
			if (chan->mask & 0x80) { pose->scale[0] += read16(p) * chan->scale[7]; p += 2; }
			if (chan->mask & 0x100) { pose->scale[1] += read16(p) * chan->scale[8]; p += 2; }
			if (chan->mask & 0x200) { pose->scale[2] += read16(p) * chan->scale[9]; p += 2; }
		}
	}

	free(chans);
}

struct model *
load_iqm_model_from_memory(unsigned char *data, char *filename)
{
	struct model *model;
	char *p;

	int flags = read32(data + 24);
	int num_text = read32(data + 28);
	int ofs_text = read32(data + 32);
	int num_meshes = read32(data + 36);
	int ofs_meshes = read32(data + 40);
	int num_vertexarrays = read32(data + 44);
	int num_vertexes = read32(data + 48);
	int ofs_vertexarrays = read32(data + 52);
	int num_triangles = read32(data + 56);
	int ofs_triangles = read32(data + 60);
	int ofs_adjacency = read32(data + 64);
	int num_bones = read32(data + 68);
	int ofs_bones = read32(data + 72);
	int num_poses = read32(data + 76);
	int ofs_poses = read32(data + 80);
	int num_anims = read32(data + 84);
	int ofs_anims = read32(data + 88);
	int num_frames = read32(data + 92);
	int num_framechannels = read32(data + 96);
	int ofs_frames = read32(data + 100);
	int ofs_bounds = read32(data + 104);
	int num_comment = read32(data + 108);
	int ofs_comment = read32(data + 112);
	int num_extensions = read32(data + 116);
	int ofs_extensions = read32(data + 120);

	model = malloc(sizeof *model);
	memset(model, 0, sizeof *model);

	strlcpy(model->dir, filename, NAMELEN);
	p = strrchr(model->dir, '/');
	if (!p) p = strrchr(model->dir, '\\');
	if (p) p[1] = 0; else strlcpy(model->dir, "./", NAMELEN);

	assert(num_bones == num_poses);

	if (num_vertexarrays && num_vertexes && num_triangles && num_meshes) {
		load_vertex_arrays(model, data, ofs_vertexarrays, num_vertexarrays, num_vertexes);
		load_triangles(model, data, ofs_triangles, num_triangles);
		load_meshes(model, data, ofs_text, ofs_meshes, num_meshes);
	}
	if (num_bones)
		load_bones(model, data, ofs_text, ofs_bones, ofs_poses, num_bones);
	if (num_anims)
		load_anims(model, data, ofs_text, ofs_anims, num_anims);
	if (num_frames)
		load_frames(model, data, ofs_poses, ofs_frames, ofs_bounds,	
			num_frames, num_framechannels);

	model->outpos = model->pos;
	model->outnorm = model->norm;
	model->outbone = NULL;
	model->outskin = NULL;

	return model;
}

struct model *
load_iqm_model(char *filename)
{
	struct model *model;
	unsigned char *data;
	unsigned char hdr[16+27*4];
	int filesize;
	FILE *file;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "cannot open model '%s'\n", filename);
		return NULL;
	}

	printf("loading iqm model '%s'\n", filename);

	fread(&hdr, 1, sizeof hdr, file);
	if (memcmp(hdr, IQM_MAGIC, 16)) {
		fprintf(stderr, "not an IQM file '%s'\n", filename);
		fclose(file);
		return NULL;
	}
	if (read32(hdr+16) != IQM_VERSION) {
		fprintf(stderr, "unknown IQM version '%s'\n", filename);
		fclose(file);
		return NULL;
	}

	filesize = read32(hdr+20);
	data = malloc(filesize);
	memcpy(data, hdr, sizeof hdr);
	fread(data + sizeof hdr, 1, filesize - sizeof hdr, file);
	fclose(file);
	model = load_iqm_model_from_memory(data, filename);
	free(data);
	return model;
}

void
animate_iqm_model(struct model *model, int anim, int frame)
{
	struct pose *poses;
	float *dstpos, *dstnorm, *srcpos, *srcnorm;
	unsigned char *idx, *w;
	int i, k, n;

	if (!model->num_bones || !model->num_anims)
		return;

	if (model->outpos == model->pos)
		model->outpos = malloc(model->num_verts * 3 * sizeof(float));
	if (model->outnorm == model->outnorm)
		model->outnorm = malloc(model->num_verts * 3 * sizeof(float));
	if (!model->outbone) {
		model->outbone = malloc(model->num_bones * sizeof(float[4][4]));
		model->outskin = malloc(model->num_bones * sizeof(float[4][4]));
	}

	frame %= model->anims[anim].count;
	poses = model->poses[model->anims[anim].first + frame];

	for (i = 0; i < model->num_bones; i++) {
		int parent = model->bones[i].parent;
		struct pose *pose = poses + i;
		float44_from_quat_vec(model->outbone[i], pose->rotate, pose->translate);
		if (parent >= 0)
			float44_mul(model->outbone[i], model->outbone[i], model->outbone[parent]);
		float44_mul(model->outskin[i], model->bones[i].inv_bind_matrix, model->outbone[i]);
	}

	idx = model->blend_index;
	w = model->blend_weight;
	srcpos = model->pos;
	srcnorm = model->norm;
	dstpos = model->outpos;
	dstnorm = model->outnorm;

	for (i = 0; i < model->num_verts; i++) {
		for (n = 0; n < 3; n++) {
			dstpos[n] = 0;
			dstnorm[n] = 0;
		}
		for (k = 0; k < 4; k++) {
			float (*m)[4] = model->outskin[idx[k]];
			if (w[k] > 0) {
				float pv[3], nv[3];
				float44_vec_mul(pv, m, srcpos);
				float44_vec_mul_n(nv, m, srcnorm);
				for (n = 0; n < 3; n++) {
					dstpos[n] += pv[n] * w[k];
					dstnorm[n] += nv[n] * w[k];
				}
			}
		}
		for (n = 0; n < 3; n++) {
			dstpos[n] *= 1/255.0f;
			dstnorm[n] *= 1/255.0f;
		}
		srcpos+=3; srcnorm+=3;
		dstpos+=3; dstnorm+=3;
		idx += 4; w += 4;
	}
}

void
draw_iqm_model(struct model *model)
{
	int i;

	glVertexPointer(3, GL_FLOAT, 0, model->outpos);
	if (model->norm) glNormalPointer(GL_FLOAT, 0, model->outnorm);
	if (model->texcoord) glTexCoordPointer(2, GL_FLOAT, 0, model->texcoord);
	// if (model->color) glColorPointer(4, GL_BYTE, 0, model->color);

	glEnableClientState(GL_VERTEX_ARRAY);
	if (model->norm) glEnableClientState(GL_NORMAL_ARRAY);
	if (model->texcoord) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// if (model->color) glEnableClientState(GL_COLOR_ARRAY);

	for (i = 0; i < model->num_meshes; i++) {
		struct mesh *mesh = model->meshes + i;
		glBindTexture(GL_TEXTURE_2D, mesh->material);
		glDrawElements(GL_TRIANGLES, 3 * mesh->count,
			GL_UNSIGNED_INT, &model->tris[mesh->first*3]);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	if (model->norm) glDisableClientState(GL_NORMAL_ARRAY);
	if (model->texcoord) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	// if (model->color) glDisableClientState(GL_COLOR_ARRAY);

	glColor3f(1,1,1);
}

void
draw_iqm_bones(struct model *model)
{
	int i;

	glBegin(GL_LINES);

	// bind pose in green
	for (i = 0; i < model->num_bones; i++) {
		struct bone *bone = model->bones + i;
		if (bone->parent >= 0) {
			struct bone *pb = model->bones + bone->parent;
			glColor3f(0, 1, 0);
			glVertex3f(pb->bind_matrix[0][3], pb->bind_matrix[1][3], pb->bind_matrix[2][3]);
		} else {
			glColor3f(0, 1, 0.5);
			glVertex3f(0, 0, 0);
		}
		glVertex3f(bone->bind_matrix[0][3], bone->bind_matrix[1][3], bone->bind_matrix[2][3]);
	}

	// current pose in red
	if (model->outbone) {
		for (i = 0; i < model->num_bones; i++) {
			struct bone *bone = model->bones + i;
			if (bone->parent >= 0) {
				float (*p)[4] = model->outbone[bone->parent];
				glColor3f(1, 0, 0);
				glVertex3f(p[0][3], p[1][3], p[2][3]);
			} else {
				glColor3f(1, 0.5, 0);
				glVertex3f(0, 0, 0);
			}
			float (*m)[4] = model->outbone[i];
			glVertex3f(m[0][3], m[1][3], m[2][3]);
		}
	}

	glEnd();
}
