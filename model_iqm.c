#include "mio.h"
#include "iqm.h"

// Simple loader assumes little-endian and 4-byte ints!

static void error(char *filename, char *msg)
{
	fprintf(stderr, "error: %s: '%s'\n", msg, filename);
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

static int load_iqm_material(char *dir, char *name, char *ext, int srgb)
{
	char filename[1024], *s;
	s = strrchr(name, '+');
	if (s) s++; else s = name;
	if (dir[0]) {
		strlcpy(filename, dir, sizeof filename);
		strlcat(filename, "/", sizeof filename);
		strlcat(filename, s, sizeof filename);
		strlcat(filename, ext, sizeof filename);
	} else {
		strlcpy(filename, s, sizeof filename);
		strlcat(filename, ext, sizeof filename);
	}
	return load_texture(filename, srgb);
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

	strlcpy(dir, filename, sizeof dir);
	p = strrchr(dir, '/');
	if (!p) p = strrchr(dir, '\\');
	if (p) p[0] = 0; else strlcpy(dir, "", sizeof dir);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) { error(filename, "bad iqm magic"); return NULL; }
	if (iqm->version != IQM_VERSION) { error(filename, "bad iqm version"); return NULL; }
	if (iqm->filesize > len) { error(filename, "bad iqm file size"); return NULL; }
	if (iqm->num_vertexes > 0xffff) { error(filename, "too many vertices in iqm"); return NULL; }
	if (iqm->num_joints > MAXBONE) { error(filename, "too many bones in iqm"); return NULL; }

	struct model *model = malloc(sizeof *model);
	model->mesh_count = iqm->num_meshes;
	model->bone_count = iqm->num_joints;

	model->mesh = malloc(model->mesh_count * sizeof *model->mesh);

	for (i = 0; i < model->mesh_count; i++) {
		char *material = text + iqmmesh[i].material;
		model->mesh[i].diffuse = load_iqm_material(dir, material, ".png", 1);
		model->mesh[i].specular = load_iqm_material(dir, material, ".s.png", 0);
		model->mesh[i].ghost = !!strstr(material, "ghost+");
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

	calc_matrix_from_pose(model->bind_matrix, model->bind_pose, model->bone_count);
	calc_abs_matrix(model->abs_bind_matrix, model->bind_matrix, model->parent, model->bone_count);
	calc_inv_matrix(model->inv_bind_matrix, model->abs_bind_matrix, model->bone_count);

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

	return model;
}

void extract_pose(struct pose *pose, struct animation *anim, int frame)
{
	if (frame < 0) frame = 0;
	if (frame >= anim->frame_count) frame = anim->frame_count - 1;
	memcpy(pose, anim->frame + anim->bone_count * frame, sizeof(struct pose) * anim->bone_count);
}

struct animation *load_iqm_animation_from_memory(char *filename, unsigned char *data, int len)
{
	struct iqmheader *iqm = (void*) data;
	char *text = (void*) &data[iqm->ofs_text];
	struct iqmjoint *iqmjoint = (void*) &data[iqm->ofs_joints];
	struct iqmpose *iqmpose = (void*) &data[iqm->ofs_poses];
	struct iqmanim *iqmanim = (void*) &data[iqm->ofs_anims];
	struct pose *pose;
	unsigned short *s;
	int i, k;

	fprintf(stderr, "loading iqm animation '%s'\n", filename);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) { error(filename, "bad iqm magic"); return NULL; }
	if (iqm->version != IQM_VERSION) { error(filename, "bad iqm version"); return NULL; }
	if (iqm->filesize > len) { error(filename, "bad iqm file size"); return NULL; }
	if (iqm->num_joints > MAXBONE) { error(filename, "too many bones in iqm"); return NULL; }
	if (iqm->num_joints < iqm->num_poses) { error(filename, "bad joint/pose data"); return NULL; }

	if (iqm->num_anims == 0)
		return NULL;

	fprintf(stderr, "\t%d bones; %d channels; %d animations; %d frames\n",
		iqm->num_joints, iqm->num_framechannels, iqm->num_anims, iqm->num_frames);

	struct animation *anim = malloc(sizeof *anim);
	anim->bone_count = iqm->num_joints;
	anim->frame_count = iqmanim->num_frames;
	anim->flags = iqmanim->flags;

	anim->frame = malloc(anim->frame_count * anim->bone_count * sizeof(struct pose));

	for (i = 0; i < anim->bone_count; i++) {
		strlcpy(anim->bone_name[i], text + iqmjoint[i].name, sizeof anim->bone_name[0]);
		anim->parent[i] = iqmpose[i].parent;
		memcpy(&anim->bind_pose[i], iqmjoint[i].translate, 10*sizeof(float));
	}

	calc_matrix_from_pose(anim->bind_matrix, anim->bind_pose, anim->bone_count);
	//calc_inv_matrix(anim->inv_loc_bind_matrix, anim->bind_matrix, anim->bone_count);
	calc_abs_matrix(anim->abs_bind_matrix, anim->bind_matrix, anim->parent, anim->bone_count);

	s = (void*) &data[iqm->ofs_frames];
	for (k = 0; k < anim->frame_count; k++) {
		pose = anim->frame + anim->bone_count * k;
		for (i = 0; i < anim->bone_count; i++) {
			float *offset = iqmpose[i].channeloffset;
			float *scale = iqmpose[i].channelscale;
			unsigned int mask = iqmpose[i].mask;
			pose[i].translate[0] = (mask & 0x01) ? offset[0] + *s++ * scale[0] : offset[0];
			pose[i].translate[1] = (mask & 0x02) ? offset[1] + *s++ * scale[1] : offset[1];
			pose[i].translate[2] = (mask & 0x04) ? offset[2] + *s++ * scale[2] : offset[2];
			pose[i].rotate[0] = (mask & 0x08) ? offset[3] + *s++ * scale[3] : offset[3];
			pose[i].rotate[1] = (mask & 0x10) ? offset[4] + *s++ * scale[4] : offset[4];
			pose[i].rotate[2] = (mask & 0x20) ? offset[5] + *s++ * scale[5] : offset[5];
			pose[i].rotate[3] = (mask & 0x40) ? offset[6] + *s++ * scale[6] : offset[6];
			pose[i].scale[0] = (mask & 0x80) ? offset[7] + *s++ * scale[7] : offset[7];
			pose[i].scale[1] = (mask & 0x100) ? offset[8] + *s++ * scale[8] : offset[8];
			pose[i].scale[2] = (mask & 0x200) ? offset[9] + *s++ * scale[9] : offset[9];
		}
	}

	// TODO: recalc anim pose as delta from skeleton / bind pose here

	// HACK: looping animations, last and first frame
	if (anim->frame_count > 1)
		anim->frame_count--;

	return anim;
}
