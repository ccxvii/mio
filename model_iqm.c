#include "mio.h"
#include "iqm.h"

// Simple loader assumes little-endian and 4-byte ints!

static void error(const char *filename, char *msg)
{
	warn("error: %s: '%s'", msg, filename);
}

static int enum_of_type(int type, char *text)
{
	switch (type) {
	case IQM_POSITION: return ATT_POSITION;
	case IQM_TEXCOORD: return ATT_TEXCOORD;
	case IQM_NORMAL: return ATT_NORMAL;
	case IQM_TANGENT: return ATT_TANGENT;
	case IQM_BLENDINDEXES: return ATT_BLEND_INDEX;
	case IQM_BLENDWEIGHTS: return ATT_BLEND_WEIGHT;
	case IQM_COLOR: return ATT_COLOR;
	}
	if (type >= IQM_CUSTOM) {
		text = text + type - IQM_CUSTOM;
		if (!strcmp(text, "lightmap")) return ATT_LIGHTMAP;
		if (!strcmp(text, "wind")) return ATT_WIND;
		if (!strcmp(text, "splat")) return ATT_SPLAT;
	}
	return -1;
}

static int use_vertex_array(int type, char *text)
{
	return enum_of_type(type, text) >= 0;
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

static mat4 loc_bind_matrix[MAXBONE];
static mat4 abs_bind_matrix[MAXBONE];

struct model *load_iqm_from_memory(const char *filename, unsigned char *data, int len)
{
	struct iqmheader *iqm = (void*)data;
	struct iqmvertexarray *vertexarrays = (void*)(data + iqm->ofs_vertexarrays);
	struct iqmmesh *iqmesh = (void*)(data + iqm->ofs_meshes);
	struct iqmjoint *iqjoint = (void*)(data + iqm->ofs_joints);
	struct iqmpose *iqpose = (void*)(data + iqm->ofs_poses);
	struct iqmanim *iqanim = (void*)(data + iqm->ofs_anims);
	char *text = (void*)(data + iqm->ofs_text);
	unsigned short *frames = (void*)(data + iqm->ofs_frames);
	struct skel *skel = NULL;
	struct mesh *mesh = NULL;
	struct anim *anim_head = NULL;
	int i, f, k, total;

	char *p;
	char dir[256];
	strlcpy(dir, filename, sizeof dir);
	p = strrchr(dir, '/');
	if (!p) p = strrchr(dir, '\\');
	if (p) p[0] = 0; else strlcpy(dir, "", sizeof dir);

	assert(sizeof(int) == 4);

	if (memcmp(iqm->magic, IQM_MAGIC, 16)) { error(filename, "bad iqm magic"); return NULL; }
	if (iqm->version != IQM_VERSION) { error(filename, "bad iqm version"); return NULL; }
	if (iqm->filesize > len) { error(filename, "bad iqm file size"); return NULL; }
	if (iqm->num_vertexes > 0xffff) { error(filename, "too many vertices in iqm"); return NULL; }
	if (iqm->num_joints > MAXBONE) { error(filename, "too many bones in iqm"); return NULL; }
	if (iqm->num_anims && iqm->num_poses != iqm->num_joints) { error(filename, "bad joint/pose data"); return NULL; }

	if (iqm->num_joints) {
		skel = malloc(sizeof(struct skel));
		skel->tag = TAG_SKEL;
		skel->count = iqm->num_joints;
		for (i = 0; i < iqm->num_joints; i++) {
			strlcpy(skel->name[i], text + iqjoint[i].name, sizeof skel->name[0]);
			skel->parent[i] = iqjoint[i].parent;
			memcpy(skel->pose[i].position, iqjoint[i].translate, 3 * sizeof(float));
			memcpy(skel->pose[i].rotation, iqjoint[i].rotate, 4 * sizeof(float));
			memcpy(skel->pose[i].scale, iqjoint[i].scale, 3 * sizeof(float));
		}
	}

	if (iqm->num_meshes) {
		mesh = malloc(sizeof(struct mesh));
		mesh->tag = TAG_MESH;
		mesh->enabled = 0;

		if (skel) {
			mesh->skel = skel;
			mesh->inv_bind_matrix = malloc(sizeof(mat4) * skel->count);
			calc_matrix_from_pose(loc_bind_matrix, skel->pose, skel->count);
			calc_abs_matrix(abs_bind_matrix, loc_bind_matrix, skel->parent, skel->count);
			calc_inv_matrix(mesh->inv_bind_matrix, abs_bind_matrix, skel->count);
		} else {
			mesh->skel = NULL;
			mesh->inv_bind_matrix = NULL;
		}

		mesh->count = iqm->num_meshes;
		mesh->part = malloc(iqm->num_meshes * sizeof(struct part));
		for (i = 0; i < iqm->num_meshes; i++) {
			mesh->part[i].material = load_material(dir, text + iqmesh[i].material);
			mesh->part[i].first = iqmesh[i].first_triangle * 3;
			mesh->part[i].count = iqmesh[i].num_triangles * 3;
		}

		glGenVertexArrays(1, &mesh->vao);
		glGenBuffers(1, &mesh->vbo);
		glGenBuffers(1, &mesh->ibo);

		glBindVertexArray(mesh->vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

		total = 0;
		for (i = 0; i < iqm->num_vertexarrays; i++) {
			struct iqmvertexarray *va = vertexarrays + i;
			if (use_vertex_array(va->type, text))
				total += size_of_format(va->format) * va->size * iqm->num_vertexes;
		}

		glBufferData(GL_ARRAY_BUFFER, total, NULL, GL_STATIC_DRAW);

		total = 0;
		for (i = 0; i < iqm->num_vertexarrays; i++) {
			struct iqmvertexarray *va = vertexarrays + i;
			if (use_vertex_array(va->type, text)) {
				int current = size_of_format(va->format) * va->size * iqm->num_vertexes;
				int format = enum_of_format(va->format);
				int type = enum_of_type(va->type, text);
				int normalize = va->type != IQM_BLENDINDEXES;
				mesh->enabled |= 1<<type;
				glBufferSubData(GL_ARRAY_BUFFER, total, current, data + va->offset);
				glEnableVertexAttribArray(type);
				glVertexAttribPointer(type, va->size, format, normalize, 0, PTR(total));
				total += current;
			}
		}

		unsigned short *triangles = malloc(iqm->num_triangles * 3 * 2);
		flip_triangles(triangles, (void*)&data[iqm->ofs_triangles], iqm->num_triangles);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, iqm->num_triangles * 3 * 2, triangles, GL_STATIC_DRAW);
		free(triangles);
	}

	for (k = 0; k < iqm->num_anims; k++) {
		struct anim *anim = malloc(sizeof(struct anim));
		anim->tag = TAG_ANIM;
		anim->anim_map_head = NULL;
		anim->name = strdup(text + iqanim[k].name);
		anim->skel = skel;
		anim->frames = iqanim[k].num_frames;
		anim->channels = iqm->num_framechannels;
		anim->framerate = iqanim[k].framerate;
		anim->loop = iqanim[k].flags & IQM_LOOP;
		anim->data = malloc(iqanim[k].num_frames * iqm->num_framechannels * sizeof(float));

		for (i = 0; i < iqm->num_joints; i++) {
			anim->mask[i] = iqpose[i].mask;
			anim->pose[i].position[0] = iqpose[i].channeloffset[0];
			anim->pose[i].position[1] = iqpose[i].channeloffset[1];
			anim->pose[i].position[2] = iqpose[i].channeloffset[2];
			anim->pose[i].rotation[0] = iqpose[i].channeloffset[3];
			anim->pose[i].rotation[1] = iqpose[i].channeloffset[4];
			anim->pose[i].rotation[2] = iqpose[i].channeloffset[5];
			anim->pose[i].rotation[3] = iqpose[i].channeloffset[6];
			anim->pose[i].scale[0] = iqpose[i].channeloffset[7];
			anim->pose[i].scale[1] = iqpose[i].channeloffset[8];
			anim->pose[i].scale[2] = iqpose[i].channeloffset[9];
		}

		unsigned short *src = frames + iqanim[k].first_frame * iqm->num_framechannels;
		float *dst = anim->data;
		for (f = 0; f < iqanim[k].num_frames; f++) {
			for (i = 0; i < iqm->num_joints; i++) {
				unsigned int mask = iqpose[i].mask;
				float *offset = iqpose[i].channeloffset;
				float *scale = iqpose[i].channelscale;
				if (mask & 0x01) *dst++ = offset[0] + *src++ * scale[0];
				if (mask & 0x02) *dst++ = offset[1] + *src++ * scale[1];
				if (mask & 0x04) *dst++ = offset[2] + *src++ * scale[2];
				if (mask & 0x08) *dst++ = offset[3] + *src++ * scale[3];
				if (mask & 0x10) *dst++ = offset[4] + *src++ * scale[4];
				if (mask & 0x20) *dst++ = offset[5] + *src++ * scale[5];
				if (mask & 0x40) *dst++ = offset[6] + *src++ * scale[6];
				if (mask & 0x80) *dst++ = offset[7] + *src++ * scale[7];
				if (mask & 0x100) *dst++ = offset[8] + *src++ * scale[8];
				if (mask & 0x200) *dst++ = offset[9] + *src++ * scale[9];
			}
		}

		anim->next = anim_head;
		anim_head = anim;
	}

	struct model *model = malloc(sizeof(struct model));
	model->skel = skel;
	model->mesh = mesh;
	model->anim = anim_head;
	return model;
}
