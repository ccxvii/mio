#include "mio.h"

struct {
	const char *suffix;
	struct model *(*load)(const char *filename, unsigned char *data, int len);
} formats[] = {
	{ ".iqm", load_iqm_from_memory },
	{ ".iqe", load_iqe_from_memory },
	{ ".obj", load_obj_from_memory },
};

static struct cache *model_cache = NULL;

struct model *load_model(const char *name)
{
	char filename[1024];
	unsigned char *data = NULL;
	struct model *model;
	int i, len;

	model = lookup(model_cache, name);
	if (model)
		return model;

	for (i = 0; i < nelem(formats); i++) {
		strlcpy(filename, name, sizeof filename);
		strlcat(filename, formats[i].suffix, sizeof filename);
		data = load_file(filename, &len);
		if (data)
			break;
	}

	if (!data) {
		warn("error: cannot find model: '%s'", name);
		return NULL;
	}

	model = formats[i].load(filename, data, len);
	if (!model)
		warn("error: cannot load model: '%s'", filename);

	free(data);

	if (model)
		model_cache = insert(model_cache, name, model);

	if (model && model->anim) {
		struct anim *anim = model->anim;
		struct pose a_from_org, b_from_org;
		vec4 org_from_a;
		extract_frame_root(&a_from_org, anim, 0);
		extract_frame_root(&b_from_org, anim, anim->frames - 1);
		vec_sub(anim->motion.position, b_from_org.position, a_from_org.position);
		quat_conjugate(org_from_a, a_from_org.rotation);
		quat_mul(anim->motion.rotation, b_from_org.rotation, org_from_a);
		vec_div(anim->motion.scale, b_from_org.scale, a_from_org.scale);
	}

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

void extract_raw_frame_root(struct pose *pose, struct anim *anim, int frame)
{
	float *s = anim->data + anim->channels * frame;
	int mask = anim->mask[0];
	*pose = anim->pose[0];
	if (mask & 0x01) pose->position[0] = *s++;
	if (mask & 0x02) pose->position[1] = *s++;
	if (mask & 0x04) pose->position[2] = *s++;
	if (mask & 0x08) pose->rotation[0] = *s++;
	if (mask & 0x10) pose->rotation[1] = *s++;
	if (mask & 0x20) pose->rotation[2] = *s++;
	if (mask & 0x40) pose->rotation[3] = *s++;
	if (mask & 0x80) pose->scale[0] = *s++;
	if (mask & 0x100) pose->scale[1] = *s++;
	if (mask & 0x200) pose->scale[2] = *s++;
}

void extract_raw_frame(struct pose *pose, struct anim *anim, int frame)
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

void lerp_frame(struct pose *out, struct pose *a, struct pose *b, float t, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		vec_lerp(out[i].position, a[i].position, b[i].position, t);
		quat_lerp_neighbor_normalize(out[i].rotation, a[i].rotation, b[i].rotation, t);
		vec_lerp(out[i].scale, a[i].scale, b[i].scale, t);
	}
}

void extract_frame_root(struct pose *pose, struct anim *anim, float frame)
{
	int frame0 = frame;
	int frame1 = frame0 + 1;
	if (frame <= 0) {
		extract_raw_frame_root(pose, anim, 0);
		return;
	}
	if (frame1 >= anim->frames) {
		extract_raw_frame_root(pose, anim, anim->frames - 1);
		return;
	}
	float t = frame - floorf(frame);
	struct pose a, b;
	extract_raw_frame_root(&a, anim, frame0);
	extract_raw_frame_root(&b, anim, frame1);
	lerp_frame(pose, &a, &b, t, 1);
}

void extract_frame(struct pose *pose, struct anim *anim, float frame)
{
	int frame0 = frame;
	int frame1 = frame0 + 1;
	if (frame <= 0) {
		extract_raw_frame(pose, anim, 0);
		return;
	}
	if (frame1 >= anim->frames) {
		extract_raw_frame(pose, anim, anim->frames - 1);
		return;
	}
	float t = frame - floorf(frame);
	struct pose a[MAXBONE], b[MAXBONE];
	extract_raw_frame(a, anim, frame0);
	extract_raw_frame(b, anim, frame1);
	lerp_frame(pose, a, b, t, anim->skel->count);
}

static int haschildren(int *parent, int count, int x)
{
	int i;
	for (i = x; i < count; i++)
		if (parent[i] == x)
			return 1;
	return 0;
}

void draw_skel(mat4 *abs_pose_matrix, int *parent, int count)
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
