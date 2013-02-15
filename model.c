#include "mio.h"

static struct cache *model_cache = NULL;

struct model *load_model(const char *filename)
{
	struct model *model;
	unsigned char *data;
	int len;

	model = lookup(model_cache, filename);
	if (model)
		return model;

	data = load_file(filename, &len);
	if (!data) {
		fprintf(stderr, "error: cannot load model file: '%s'\n", filename);
		return NULL;
	}

	model = NULL;
	if (strstr(filename, ".iqm")) model = load_iqm_from_memory(filename, data, len);
	if (strstr(filename, ".iqe")) model = load_iqe_from_memory(filename, data, len);
	if (strstr(filename, ".obj")) model = load_obj_from_memory(filename, data, len);
	if (!model)
		fprintf(stderr, "error: cannot load model: '%s'\n", filename);

	free(data);

	if (model)
		model_cache = insert(model_cache, filename, model);

	return model;

}

struct skel *load_skel(const char *filename)
{
	struct model *model = load_model(filename);
	if (model)
		return model->skel;
	return NULL;
}

struct mesh *load_mesh(const char *filename)
{
	struct model *model = load_model(filename);
	if (model)
		return model->mesh;
	return NULL;
}

struct anim *load_anim(const char *filename)
{
	struct model *model = load_model(filename);
	if (model)
		return model->anim;
	return NULL;
}

/* TODO: clean up below */

void extract_pose(struct pose *pose, struct anim *anim, int frame)
{
	float *s = anim->data + anim->channels * frame;
	int i;
	for (i = 0; i < anim->skel->count; i++) {
		int mask = anim->mask[i];
		pose[i] = anim->pose[i];
		if (mask & 0x01) pose[i].position[0] = *s++;
		if (mask & 0x02) pose[i].position[1] = *s++;
		if (mask & 0x04) pose[i].position[2] = *s++;
		if (mask & 0x08) pose[i].rotation[0] = *s++;
		if (mask & 0x10) pose[i].rotation[1] = *s++;
		if (mask & 0x20) pose[i].rotation[2] = *s++;
		if (mask & 0x40) pose[i].rotation[3] = *s++;
		if (mask & 0x80) pose[i].scale[0] = *s++;
		if (mask & 0x100) pose[i].scale[1] = *s++;
		if (mask & 0x200) pose[i].scale[2] = *s++;
	}
}

static int haschildren(int *parent, int count, int x)
{
	int i;
	for (i = x; i < count; i++)
		if (parent[i] == x)
			return 1;
	return 0;
}

void draw_armature(mat4 *abs_pose_matrix, int *parent, int count)
{
	vec3 x = { 0.1, 0, 0 };
	int i;
	for (i = 0; i < count; i++) {
		float *a = abs_pose_matrix[i];
		if (parent[i] >= 0) {
			float *b = abs_pose_matrix[parent[i]];
			draw_line(a[12], a[13], a[14], b[12], b[13], b[14]);
		} else {
			draw_line(a[12], a[13], a[14], 0, 0, 0);
		}
		if (!haschildren(parent, count, i)) {
			vec3 b;
			mat_vec_mul(b, abs_pose_matrix[i], x);
			draw_line(a[12], a[13], a[14], b[0], b[1], b[2]);
		}
	}
}

static int findbone(char (*bone_name)[32], int count, char *name)
{
	int i;
	for (i = 0; i < count; i++)
		if (!strcmp(bone_name[i], name))
			return i;
	return -1;
}

void apply_animation(struct pose *dst_pose, struct skel *dst, struct pose *src_pose, struct skel *src)
{
	int s, d;
	for (d = 0; d < dst->count; d++) {
		s = findbone(src->name, src->count, dst->name[d]);
		if (s >= 0) {
			dst_pose[d] = src_pose[s];
		}
	}
}

static const char *ryzom_bones[] = {
	"bip01", "bip01_pelvis", "bip01_l_clavicle", "bip01_r_clavicle", "bip01_spine", "bip01_tail",
	"bip02", "bip02_pelvis",
};

void apply_animation_ryzom(struct pose *dst_pose, struct skel *dst, struct pose *src_pose, struct skel *src)
{
	int s, d, i;
	for (d = 0; d < dst->count; d++) {
		s = findbone(src->name, src->count, dst->name[d]);
		if (s >= 0) {
			for (i = 0; i < nelem(ryzom_bones); i++) {
				if (!strcmp(dst->name[d], ryzom_bones[i])) {
					dst_pose[d] = src_pose[s];
					goto next;
				}
			}
			memcpy(dst_pose[d].rotation, src_pose[s].rotation, 4*sizeof(float));
		}
next:;
	}
}
