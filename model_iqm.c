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
	float bind_matrix[16];
	float inv_bind_matrix[16];
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

	float min[3], max[3], radius;

	float (*outbone)[16];
	float (*outskin)[16];
	struct pose *outpose;

	unsigned int vbo, ibo;
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
	int i;
	model->num_verts = num_v;
	for (i = 0; i < num_va; i++) {
		int type = read32(data + ofs_va + 0);
		// int flags = read32(data + ofs_va + 4);
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
			int k;
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
	return load_texture(0, buf);
}

static void
load_meshes(struct model *model, unsigned char *data, int ofs_text, int ofs_meshes, int num_meshes)
{
	int i;
	model->num_meshes = num_meshes;
	model->meshes = malloc(num_meshes * sizeof(struct mesh));
	for (i = 0; i < num_meshes; i++) {
		char *name = (char*)data + ofs_text + read32(data + ofs_meshes + 0);
		char *material = (char*)data + ofs_text + read32(data + ofs_meshes + 4);
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
	quat_normalize(pose->rotate);
}

static void
load_bones(struct model *model, unsigned char *data, int ofs_text, int ofs_bones, int num_bones)
{
	int i;
	model->num_bones = num_bones;
	model->bones = malloc(num_bones * sizeof(struct bone));
	for (i = 0; i < num_bones; i++) {
		float m[16];
		struct bone *bone = model->bones + i;
		char *name = (char*)data + ofs_text + read32(data + ofs_bones + 0);
		bone->parent = read32(data + ofs_bones + 4);
		strlcpy(bone->name, name, NAMELEN);
		read_pose(&bone->bind_pose, data + ofs_bones + 8);
		mat_from_quat_vec(m, bone->bind_pose.rotate, bone->bind_pose.translate);
		if (bone->parent >= 0) {
			struct bone *parent = model->bones + bone->parent;
			mat_mul(bone->bind_matrix, parent->bind_matrix, m);
		} else {
			mat_copy(bone->bind_matrix, m);
		}
		mat_invert(bone->inv_bind_matrix, bone->bind_matrix);
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
		char *name = (char*)data + ofs_text + read32(data + ofs_anims + 0);
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
	int i;

//	int flags = read32(data + 24);
//	int num_text = read32(data + 28);
	int ofs_text = read32(data + 32);
	int num_meshes = read32(data + 36);
	int ofs_meshes = read32(data + 40);
	int num_vertexarrays = read32(data + 44);
	int num_vertexes = read32(data + 48);
	int ofs_vertexarrays = read32(data + 52);
	int num_triangles = read32(data + 56);
	int ofs_triangles = read32(data + 60);
//	int ofs_adjacency = read32(data + 64);
	int num_bones = read32(data + 68);
	int ofs_bones = read32(data + 72);
//	int num_poses = read32(data + 76);
	int ofs_poses = read32(data + 80);
	int num_anims = read32(data + 84);
	int ofs_anims = read32(data + 88);
	int num_frames = read32(data + 92);
	int num_framechannels = read32(data + 96);
	int ofs_frames = read32(data + 100);
	int ofs_bounds = read32(data + 104);
//	int num_comment = read32(data + 108);
//	int ofs_comment = read32(data + 112);
//	int num_extensions = read32(data + 116);
//	int ofs_extensions = read32(data + 120);

	model = malloc(sizeof *model);
	memset(model, 0, sizeof *model);

	strlcpy(model->dir, filename, NAMELEN);
	p = strrchr(model->dir, '/');
	if (!p) p = strrchr(model->dir, '\\');
	if (p) p[1] = 0; else strlcpy(model->dir, "./", NAMELEN);

	if (num_vertexarrays && num_vertexes && num_triangles && num_meshes) {
		load_vertex_arrays(model, data, ofs_vertexarrays, num_vertexarrays, num_vertexes);
		load_triangles(model, data, ofs_triangles, num_triangles);
		load_meshes(model, data, ofs_text, ofs_meshes, num_meshes);
	}
	if (num_bones)
		load_bones(model, data, ofs_text, ofs_bones, num_bones);
	if (num_anims)
		load_anims(model, data, ofs_text, ofs_anims, num_anims);
	if (num_frames)
		load_frames(model, data, ofs_poses, ofs_frames, ofs_bounds,
			num_frames, num_framechannels);

	model->min[0] = model->min[1] = model->min[2] = 1e10;
	model->max[0] = model->max[1] = model->max[2] = -1e10;
	model->radius = 0;

	for (i = 0; i < model->num_verts * 3; i += 3) {
		float x = model->pos[i];
		float y = model->pos[i+1];
		float z = model->pos[i+2];
		float r = x*x + y*y + z*z;
		if (x < model->min[0]) model->min[0] = x;
		if (y < model->min[1]) model->min[1] = y;
		if (z < model->min[2]) model->min[2] = z;
		if (x > model->max[0]) model->max[0] = x;
		if (y > model->max[1]) model->max[1] = y;
		if (z > model->max[2]) model->max[2] = z;
		if (r > model->radius) model->radius = r;
	}

	model->radius = sqrtf(model->radius);

	model->outpose = NULL;
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

float measure_iqm_radius(struct model *model)
{
	return model->radius;
}

char *
get_iqm_animation_name(struct model *model, int anim)
{
	if (!model->num_bones || !model->num_anims)
		return "none";
	if (anim < 0) anim = 0;
	if (anim >= model->num_anims) anim = model->num_anims - 1;
	return model->anims[anim].name;
}

void
animate_iqm_model(struct model *model, int anim, int frame, float t)
{
	struct pose *pose0, *pose1;
	float m[16], q[4], v[3];
	int frame0, frame1;
	int i;

	if (!model->num_bones || !model->num_anims)
		return;

	if (!model->outbone) {
		model->outbone = malloc(model->num_bones * sizeof(float[16]));
		model->outskin = malloc(model->num_bones * sizeof(float[16]));
	}

	if (anim < 0) anim = 0;
	if (anim >= model->num_anims) anim = model->num_anims - 1;

	frame0 = frame % model->anims[anim].count;
	frame1 = (frame + 1) % model->anims[anim].count;
	pose0 = model->poses[model->anims[anim].first + frame0];
	pose1 = model->poses[model->anims[anim].first + frame1];

	for (i = 0; i < model->num_bones; i++) {
		int parent = model->bones[i].parent;
		quat_lerp_neighbor_normalize(q, pose0[i].rotate, pose1[i].rotate, t);
		vec_lerp(v, pose0[i].translate, pose1[i].translate, t);
		if (parent >= 0) {
			mat_from_quat_vec(m, q, v);
			mat_mul(model->outbone[i], model->outbone[parent], m);
		} else {
			mat_from_quat_vec(model->outbone[i], q, v);
		}
		mat_mul(model->outskin[i], model->outbone[i], model->bones[i].inv_bind_matrix);
	}
}

static void make_vbo(struct model *model)
{
	int norm_ofs = 0, texcoord_ofs = 0, color_ofs = 0, blend_index_ofs = 0, blend_weight_ofs = 0;
	int n = 12;
	if (model->norm) { norm_ofs = n; n += 12; }
	if (model->texcoord) { texcoord_ofs = n; n += 8; }
	if (model->color) { color_ofs = n; n += 4; }
	if (model->blend_index && model->blend_weight) {
		blend_index_ofs = n; n += 4;
		blend_weight_ofs = n; n += 4;
	}

	glGenBuffers(1, &model->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * model->num_verts, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 12 * model->num_verts, model->pos);
	if (model->norm)
		glBufferSubData(GL_ARRAY_BUFFER, norm_ofs * model->num_verts, 12 * model->num_verts, model->norm);
	if (model->texcoord)
		glBufferSubData(GL_ARRAY_BUFFER, texcoord_ofs * model->num_verts, 8 * model->num_verts, model->texcoord);
	if (model->color)
		glBufferSubData(GL_ARRAY_BUFFER, color_ofs * model->num_verts, 4 * model->num_verts, model->color);
	if (model->blend_index && model->blend_weight) {
		glBufferSubData(GL_ARRAY_BUFFER, blend_index_ofs * model->num_verts, 4 * model->num_verts, model->blend_index);
		glBufferSubData(GL_ARRAY_BUFFER, blend_weight_ofs * model->num_verts, 4 * model->num_verts, model->blend_weight);
	}

	glGenBuffers(1, &model->ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->num_tris * 3 * sizeof(int), model->tris, GL_STATIC_DRAW);
}

void
draw_iqm_instances(struct model *model, float *trafo, int count)
{
	int i, k, n;
	int prog, loc, bi_loc, bw_loc;

	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
	loc = glGetUniformLocation(prog, "BoneMatrix");
	bi_loc = glGetAttribLocation(prog, "in_BlendIndex");
	bw_loc = glGetAttribLocation(prog, "in_BlendWeight");

	if (!model->vbo)
		make_vbo(model);

	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	glVertexAttribPointer(ATT_POSITION, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(ATT_POSITION);
	n = 12 * model->num_verts;

	if (model->norm) {
		glVertexAttribPointer(ATT_NORMAL, 3, GL_FLOAT, 0, 0, (char*)n);
		glEnableVertexAttribArray(ATT_NORMAL);
		n += 12 * model->num_verts;
	}
	if (model->texcoord) {
		glVertexAttribPointer(ATT_TEXCOORD, 2, GL_FLOAT, 0, 0, (char*)n);
		glEnableVertexAttribArray(ATT_TEXCOORD);
		n += 8 * model->num_verts;
	}
	if (model->color) {
		glVertexAttribPointer(ATT_COLOR, 4, GL_UNSIGNED_BYTE, 1, 0, (char*)n);
		glEnableVertexAttribArray(ATT_COLOR);
		n += 4 * model->num_verts;
	}

	if (loc >= 0 && bi_loc >= 0 && bw_loc >= 0) {
		if (model->outskin && model->blend_index && model->blend_weight) {
			glUniformMatrix4fv(loc, model->num_bones, 0, model->outskin[0]);
			glVertexAttribPointer(bi_loc, 4, GL_UNSIGNED_BYTE, 0, 0, (char*)n);
			n += 4 * model->num_verts;
			glVertexAttribPointer(bw_loc, 4, GL_UNSIGNED_BYTE, 1, 0, (char*)n);
			n += 4 * model->num_verts;
			glEnableVertexAttribArray(bi_loc);
			glEnableVertexAttribArray(bw_loc);
		}
	}

	for (i = 0; i < model->num_meshes; i++) {
		struct mesh *mesh = model->meshes + i;
		glBindTexture(GL_TEXTURE_2D, mesh->material);
		for (k = 0; k < count; k++) {
			// dog slow! should use our own uniforms, or instanced array
//			glPushMatrix();
//			glMultMatrixf(&trafo[k*16]);
			glDrawElements(GL_TRIANGLES, 3 * mesh->count, GL_UNSIGNED_INT, (char*)(mesh->first*12));
//			glPopMatrix();
		}
	}

	glDisableVertexAttribArray(ATT_POSITION);
	glDisableVertexAttribArray(ATT_NORMAL);
	glDisableVertexAttribArray(ATT_TEXCOORD);
	glDisableVertexAttribArray(ATT_COLOR);
	if (loc >= 0 && bi_loc >= 0 && bw_loc >= 0) {
		glDisableVertexAttribArray(bi_loc);
		glDisableVertexAttribArray(bw_loc);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
draw_iqm_model(struct model *model)
{
	float transform[16];
	mat_identity(transform);
	draw_iqm_instances(model, transform, 1);
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
			glVertex3f(pb->bind_matrix[12], pb->bind_matrix[13], pb->bind_matrix[14]);
		} else {
			glColor3f(0, 1, 0.5);
			glVertex3f(0, 0, 0);
		}
		glVertex3f(bone->bind_matrix[12], bone->bind_matrix[13], bone->bind_matrix[14]);
	}

	// current pose in red
	if (model->outbone) {
		for (i = 0; i < model->num_bones; i++) {
			struct bone *bone = model->bones + i;
			if (bone->parent >= 0) {
				float *p = model->outbone[bone->parent];
				glColor3f(1, 0, 0);
				glVertex3f(p[12], p[13], p[14]);
			} else {
				glColor3f(1, 0.5, 0);
				glVertex3f(0, 0, 0);
			}
			float *m = model->outbone[i];
			glVertex3f(m[12], m[13], m[14]);
		}
	}

	glEnd();
}
