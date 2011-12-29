#include "mio.h"
#include "iqm.h"

// Simple loader assumes little-endian and 4-byte ints!

static void error(char *filename, char *msg)
{
	fprintf(stderr, "cannot load %s: %s\n", filename, msg);
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

static void flip_triangles(unsigned short *dst, unsigned int *src, int count)
{
	while (count--) {
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst += 3;
		src += 3;
	}
}

static int load_iqm_material(char *dir, char *name)
{
	char filename[1024], *s;
	s = strrchr(name, '+');
	if (s) s++; else s = name;
	strlcpy(filename, dir, sizeof filename);
	strlcat(filename, "/", sizeof filename);
	strlcat(filename, s, sizeof filename);
	strlcat(filename, ".png", sizeof filename);
	return load_texture(0, filename, 1);
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
	char dir[256];

	fprintf(stderr, "loading iqm model '%s'\n", filename);

	strlcpy(dir, filename, sizeof dir);
	p = strrchr(dir, '/');
	if (!p) p = strrchr(dir, '\\');
	if (p) p[1] = 0; else strlcpy(dir, "./", sizeof dir);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) { error(filename, "bad iqm magic"); return NULL; }
	if (iqm->version != IQM_VERSION) { error(filename, "bad iqm version"); return NULL; }
	if (iqm->filesize > len) { error(filename, "bad iqm file size"); return NULL; }
	if (iqm->num_vertexes > 0xffff) { error(filename, "too many vertices in iqm"); return NULL; }
	if (iqm->num_joints > MAXBONE) { error(filename, "too many bones in iqm"); return NULL; }

	fprintf(stderr, "\t%d meshes; %d bones; %d vertices; %d triangles\n",
		iqm->num_meshes, iqm->num_joints, iqm->num_vertexes, iqm->num_triangles);

	struct model *model = malloc(sizeof *model);
	model->mesh_count = iqm->num_meshes;
	model->bone_count = iqm->num_joints;

	model->mesh = malloc(model->mesh_count * sizeof *model->mesh);

	for (i = 0; i < model->mesh_count; i++) {
		char *material = text + iqmmesh[i].material;
		model->mesh[i].texture = load_iqm_material(dir, material);
		model->mesh[i].alphatest = !!strstr(material, "alphatest+");
		model->mesh[i].alphagloss = !!strstr(material, "alphagloss+");
		model->mesh[i].unlit = !!strstr(material, "unlit+");
		model->mesh[i].first = iqmmesh[i].first_triangle * 3;
		model->mesh[i].count = iqmmesh[i].num_triangles * 3;
	}

	for (i = 0; i < model->bone_count; i++) {
		strlcpy(model->bone_name[i], text + joints[i].name, sizeof model->bone_name[0]);
		model->parent[i] = joints[i].parent;
		memcpy(model->bind_pose[i].translate, joints[i].translate, 3*sizeof(float));
		memcpy(model->bind_pose[i].rotate, joints[i].rotate, 4*sizeof(float));
		memcpy(model->bind_pose[i].scale, joints[i].scale, 3*sizeof(float));
	}

	calc_pose_matrix(model->bind_matrix, model->bind_pose, model->bone_count);
	calc_abs_pose_matrix(model->abs_bind_matrix, model->bind_matrix, model->parent, model->bone_count);
	calc_inv_bind_matrix(model->inv_bind_matrix, model->abs_bind_matrix, model->bone_count);

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

struct animation *load_iqm_animation_from_memory(char *filename, unsigned char *data, int len)
{
	struct iqmheader *iqm = (void*) data;
	char *text = (void*) &data[iqm->ofs_text];
	struct iqmjoint *iqmjoint = (void*) &data[iqm->ofs_joints];
	struct iqmpose *iqmpose = (void*) &data[iqm->ofs_poses];
	struct iqmanim *iqmanim = (void*) &data[iqm->ofs_anims];
	int i, k;
	unsigned short *s;
	float *p;

	fprintf(stderr, "loading iqm animation '%s'\n", filename);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) { error(filename, "bad iqm magic"); return NULL; }
	if (iqm->version != IQM_VERSION) { error(filename, "bad iqm version"); return NULL; }
	if (iqm->filesize > len) { error(filename, "bad iqm file size"); return NULL; }
	if (iqm->num_joints > MAXBONE) { error(filename, "too many bones in iqm"); return NULL; }
	if (iqm->num_joints != iqm->num_poses) { error(filename, "bad joint/pose data"); return NULL; }

	if (iqm->num_anims == 0)
		return NULL;

	fprintf(stderr, "\t%d bones; %d channels; %d animations; %d frames\n",
		iqm->num_joints, iqm->num_framechannels, iqm->num_anims, iqm->num_frames);

	struct animation *anim = malloc(sizeof *anim);
	anim->bone_count = iqm->num_joints;
	anim->frame_size = iqm->num_framechannels;
	anim->frame_count = iqmanim->num_frames;
	anim->flags = iqmanim->flags;

	anim->frame = malloc(anim->frame_count * anim->frame_size * sizeof(float));

	for (i = 0; i < anim->bone_count; i++) {
		strlcpy(anim->bone_name[i], text + iqmjoint[i].name, sizeof anim->bone_name[0]);
		anim->parent[i] = iqmpose[i].parent;
		anim->mask[i] = iqmpose[i].mask;
		memcpy(&anim->offset[i], iqmpose[i].channeloffset, 10*sizeof(float));
		memcpy(&anim->bind_pose[i], iqmjoint[i].translate, 10*sizeof(float));

		int mask = anim->mask[i];
		if (0 && mask) {
			fprintf(stderr, "anim chan %s:", anim->bone_name[i]);
			if ((mask & 0x01) || (mask & 0x02) || (mask & 0x04))
				fprintf(stderr, " translate %g", vec_length(iqmpose[i].channelscale+0)*65535);
			if ((mask & 0x08) || (mask & 0x10) || (mask & 0x20) || (mask & 0x40))
				fprintf(stderr, " rotate %g", vec_length(iqmpose[i].channelscale+3)*65535);
			if ((mask & 0x80) || (mask & 0x100) || (mask & 0x200))
				fprintf(stderr, " scale %g", vec_length(iqmpose[i].channelscale+7)*65535);
			fprintf(stderr, "\n");
		}
	}

	calc_pose_matrix(anim->bind_matrix, anim->bind_pose, anim->bone_count);
	calc_abs_pose_matrix(anim->abs_bind_matrix, anim->bind_matrix, anim->parent, anim->bone_count);

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
	data = load_file(filename, &len);
	if (!data) return NULL;
	anim = load_iqm_animation_from_memory(filename, data, len);
	free(data);
	return anim;
}
