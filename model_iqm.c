#include "mio.h"
#include "iqm.h"

// Simple loader assumes little-endian and 4-byte ints!

#define FLIP

static void die(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

static int use_vertex_array(int format)
{
	switch (format) {
	case IQM_POSITION: return 1;
	case IQM_TEXCOORD: return 1;
	case IQM_NORMAL: return 1;
	case IQM_TANGENT: return 0;
	case IQM_BLENDINDEXES: return 1;
	case IQM_BLENDWEIGHTS: return 1;
	case IQM_COLOR: return 0;
	}
	return 0;
}

static int size_of_format(int format)
{
	switch (format) {
	case IQM_BYTE: return 1;
	case IQM_UBYTE: return 1;
	case IQM_SHORT: return 2;
	case IQM_USHORT: return 2;
	case IQM_INT: return 4;
	case IQM_UINT: return 4;
	case IQM_HALF: return 2;
	case IQM_FLOAT: return 4;
	case IQM_DOUBLE: return 8;
	}
	return 0;
}

static int enum_of_format(int format)
{
	switch (format) {
	case IQM_BYTE: return GL_BYTE;
	case IQM_UBYTE: return GL_UNSIGNED_BYTE;
	case IQM_SHORT: return GL_SHORT;
	case IQM_USHORT: return GL_UNSIGNED_SHORT;
	case IQM_INT: return GL_INT;
	case IQM_UINT: return GL_UNSIGNED_INT;
	case IQM_HALF: return GL_HALF_FLOAT;
	case IQM_FLOAT: return GL_FLOAT;
	case IQM_DOUBLE: return GL_DOUBLE;
	}
	return 0;
}

static void flip_normals(float *p, int count)
{
#ifdef FLIP
	count = count * 3;
	while (count--) { *p = -*p; p++; }
#endif
}

static void flip_triangles(unsigned short *dst, unsigned int *src, int count)
{
	while (count--) {
#ifdef FLIP
		dst[0] = src[0];
		dst[1] = src[2];
		dst[2] = src[1];
#else
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
#endif
		dst += 3;
		src += 3;
	}
}

static int load_iqm_material(char *dir, char *name)
{
	char buf[256], *p;
	strlcpy(buf, dir, sizeof buf);
	strlcat(buf, name, sizeof buf);
	p = strrchr(buf, ',');
	if (p) strlcpy(p, ".png", sizeof buf - (p-buf));
	return load_texture(0, buf, 1);
}

struct model *load_iqm_model_from_memory(char *filename, unsigned char *data, int len)
{
	struct iqmheader *iqm = (void*) data;
	char *text = (void*) &data[iqm->ofs_text];
	struct iqmmesh *iqmmesh = (void*) &data[iqm->ofs_meshes];
	struct iqmvertexarray *vertexarrays = (void*) &data[iqm->ofs_vertexarrays];
	struct iqmjoint *joints = (void*) &data[iqm->ofs_joints];
	int i, total;
	char *p;
	mat4 local_matrix, world_matrix[MAXBONE];
	char dir[256];

	printf("loading iqm model '%s'\n", filename);

	strlcpy(dir, filename, sizeof dir);
	p = strrchr(dir, '/');
	if (!p) p = strrchr(dir, '\\');
	if (p) p[1] = 0; else strlcpy(dir, "./", sizeof dir);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) die("bad iqm magic");
	if (iqm->version != IQM_VERSION) die("bad iqm version");
	if (iqm->filesize > len) die("bad iqm file size");
	if (iqm->num_vertexes > 0xffff) die("too many vertices in iqm");
	if (iqm->num_joints > MAXBONE) die("too many bones in iqm");

	printf("\t%d meshes; %d bones; %d vertices; %d triangles\n",
		iqm->num_meshes, iqm->num_joints, iqm->num_vertexes, iqm->num_triangles);

	struct model *model = malloc(sizeof *model);
	model->mesh_count = iqm->num_meshes;
	model->bone_count = iqm->num_joints;

	model->mesh = malloc(model->mesh_count * sizeof *model->mesh);
	model->bone = malloc(model->bone_count * sizeof *model->bone);
	model->bind_pose = malloc(model->bone_count * sizeof *model->bind_pose);

	for (i = 0; i < model->mesh_count; i++) {
		model->mesh[i].texture = load_iqm_material(dir, text + iqmmesh[i].material);
		model->mesh[i].first = iqmmesh[i].first_triangle * 3;
		model->mesh[i].count = iqmmesh[i].num_triangles * 3;
	}

	for (i = 0; i < model->bone_count; i++) {
		struct bone *bone = model->bone + i;
		struct pose *pose = model->bind_pose + i;
		strlcpy(bone->name, text + joints[i].name, sizeof bone->name);
		bone->parent = joints[i].parent;
		memcpy(pose->translate, joints[i].translate, 3*sizeof(float));
		memcpy(pose->rotate, joints[i].rotate, 4*sizeof(float));
		memcpy(pose->scale, joints[i].scale, 3*sizeof(float));
		if (bone->parent >= 0) {
			mat_from_pose(local_matrix, pose->translate, pose->rotate, pose->scale);
			mat_mul(world_matrix[i], world_matrix[bone->parent], local_matrix);
		} else {
			mat_from_pose(world_matrix[i], pose->translate, pose->rotate, pose->scale);
		}
		mat_invert(bone->inv_bind_matrix, world_matrix[i]);
	}

	glGenVertexArrays(1, &model->vao);
	glGenBuffers(1, &model->vbo);
	glGenBuffers(1, &model->ibo);

	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);

	total = 0;
	for (i = 0; i < iqm->num_vertexarrays; i++) {
		struct iqmvertexarray *va = vertexarrays + i;
		if (use_vertex_array(va->type)) {
			total += size_of_format(va->format) * va->size * iqm->num_vertexes;
		}
	}

	glBufferData(GL_ARRAY_BUFFER, total, NULL, GL_STATIC_DRAW);

	total = 0;
	for (i = 0; i < iqm->num_vertexarrays; i++) {
		struct iqmvertexarray *va = vertexarrays + i;
		if (use_vertex_array(va->type)) {
			int current = size_of_format(va->format) * va->size * iqm->num_vertexes;
			int format = enum_of_format(va->format);
			int normalize = va->type != IQM_BLENDINDEXES;
			if (va->type == IQM_NORMAL && va->format == IQM_FLOAT && va->size == 3)
				flip_normals((float*)&data[va->offset], iqm->num_vertexes);
			glBufferSubData(GL_ARRAY_BUFFER, total, current, &data[va->offset]);
			glEnableVertexAttribArray(va->type);
			glVertexAttribPointer(va->type, va->size, format, normalize, 0, (void*)total);
			total += current;
		}
	}

	unsigned short *triangles = malloc(iqm->num_triangles * 3*2);
	flip_triangles(triangles, (void*)&data[iqm->ofs_triangles], iqm->num_triangles);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, iqm->num_triangles * 3*2, triangles, GL_STATIC_DRAW);
	free(triangles);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return model;
}

struct animation *load_iqm_animation_from_memory(unsigned char *data, int len)
{
	struct iqmheader *iqm = (void*) data;
	char *text = (void*) &data[iqm->ofs_text];
	struct iqmjoint *iqmjoint = (void*) &data[iqm->ofs_joints];
	struct iqmpose *iqmpose = (void*) &data[iqm->ofs_poses];
	struct iqmanim *iqmanim = (void*) &data[iqm->ofs_anims];
	int i, k;
	unsigned short *s;
	float *p;

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) die("bad iqm magic");
	if (iqm->version != IQM_VERSION) die("bad iqm version");
	if (iqm->filesize > len) die("bad iqm file size");
	if (iqm->num_joints > MAXBONE) die("too many bones in iqm");
	if (iqm->num_joints != iqm->num_poses) die("bad joint/pose data");

	if (iqm->num_anims == 0)
		return NULL;

	printf("\t%d bones; %d channels; %d animations; %d frames\n",
		iqm->num_joints, iqm->num_framechannels, iqm->num_anims, iqm->num_frames);

	struct animation *anim = malloc(sizeof *anim);
	anim->bone_count = iqm->num_joints;
	anim->frame_size = iqm->num_framechannels;
	anim->frame_count = iqmanim->num_frames;
	anim->flags = iqmanim->flags;

	anim->chan = malloc(anim->bone_count * sizeof *anim->chan);
	anim->frame = malloc(anim->frame_count * anim->frame_size * sizeof(float));

	for (i = 0; i < anim->bone_count; i++) {
		struct chan *chan = anim->chan + i;
		strlcpy(chan->name, text + iqmjoint[i].name, sizeof chan->name);
		chan->parent = iqmpose[i].parent;
		chan->mask = iqmpose[i].mask;
		memcpy(&chan->pose, iqmpose[i].channeloffset, 10*sizeof(float));
	}

	p = anim->frame;
	s = (void*) &data[iqm->ofs_frames];
	for (k = 0; k < anim->frame_count; k++) {
		for (i = 0; i < anim->bone_count; i++) {
			float *offset = iqmpose[i].channeloffset;
			float *scale = iqmpose[i].channelscale;
			unsigned int mask = iqmpose[i].mask;
			if (mask & 0x01) *p++ = offset[0] + *s++ * scale[0];
			if (mask & 0x02) *p++ = offset[1] + *s++ * scale[1];
			if (mask & 0x04) *p++ = offset[2] + *s++ * scale[2];
			if (mask & 0x08) *p++ = offset[3] + *s++ * scale[3];
			if (mask & 0x10) *p++ = offset[4] + *s++ * scale[4];
			if (mask & 0x20) *p++ = offset[5] + *s++ * scale[5];
			if (mask & 0x40) *p++ = offset[6] + *s++ * scale[6];
			if (mask & 0x80) *p++ = offset[7] + *s++ * scale[7];
			if (mask & 0x100) *p++ = offset[8] + *s++ * scale[8];
			if (mask & 0x200) *p++ = offset[9] + *s++ * scale[9];
		}
	}

	return anim;
}

struct model *load_iqm_model(char *filename)
{
	struct model *model;
	unsigned char *data;
	int len;
	data = load_file(filename, &len);
	if (!data) return NULL;
	model = load_iqm_model_from_memory(filename, data, len);
	free(data);
	return model;
}

struct animation *load_iqm_animation(char *filename)
{
	struct animation *anim;
	unsigned char *data;
	int len;
	printf("loading iqm animation '%s'\n", filename);
	data = load_file(filename, &len);
	if (!data) return NULL;
	anim = load_iqm_animation_from_memory(data, len);
	free(data);
	return anim;
}

#if 0

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
			mat_from_pose(m, q, v);
			mat_mul(model->outbone[i], model->outbone[parent], m);
		} else {
			mat_from_pose(model->outbone[i], q, v);
		}
		mat_mul(model->outskin[i], model->outbone[i], model->bones[i].inv_bind_matrix);
	}
}

#endif
