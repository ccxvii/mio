#include "mio.h"

static int find_bone(struct skel *skel, const char *name)
{
	int i;
	for (i = 0; i < skel->count; i++)
		if (!strcmp(skel->name[i], name))
			return i;
	return -1;
}

void pose_lerp(struct pose *p, const struct pose *a, const struct pose *b, float t)
{
	vec_lerp(p->position, a->position, b->position, t);
	quat_lerp_neighbor_normalize(p->rotation, a->rotation, b->rotation, t);
	vec_lerp(p->scale, a->scale, b->scale, t);
}

void init_transform(struct transform *trafo)
{
	vec_init(trafo->position, 0, 0, 0);
	quat_init(trafo->rotation, 0, 0, 0, 1);
	vec_init(trafo->scale, 1, 1, 1);
	mat_identity(trafo->matrix);
}

void init_skelpose(struct skelpose *skelpose, struct skel *skel)
{
	int i;
	skelpose->skel = skel;
	for (i = 0; i < skel->count; i++)
		skelpose->pose[i] = skel->pose[i];
}

void init_lamp(struct lamp *lamp)
{
	lamp->type = LAMP_POINT;
	vec_init(lamp->color, 1, 1, 1);
	lamp->energy = 1;
	lamp->distance = 25;
	lamp->spot_angle = 45;
}

void update_transform(struct transform *transform)
{
	mat_from_pose(transform->matrix, transform->position, transform->rotation, transform->scale);
}

void update_transform_parent(struct transform *transform, struct transform *parent)
{
	mat4 local_matrix;
	mat_from_pose(local_matrix, transform->position, transform->rotation, transform->scale);
	mat_mul44(transform->matrix, parent->matrix, local_matrix);
}

void calc_pose(mat4 out, struct skel *skel, struct pose *pose, int bone)
{
	int parent = skel->parent[bone];
	if (parent == -1) {
		mat_from_pose(out, pose[bone].position, pose[bone].rotation, pose[bone].scale);
	} else {
		mat4 par, loc;
		calc_pose(par, skel, pose, parent);
		mat_from_pose(loc, pose[bone].position, pose[bone].rotation, pose[bone].scale);
		mat_mul44(out, par, loc);
	}
}

void update_transform_parent_skel(struct transform *transform,
	struct transform *parent, struct skelpose *skelpose, const char *bone)
{
	mat4 local_matrix, pose_matrix, m;
	int i = find_bone(skelpose->skel, bone);
	mat_from_pose(local_matrix, transform->position, transform->rotation, transform->scale);
	calc_pose(pose_matrix, skelpose->skel, skelpose->pose, i);
	mat_mul44(m, pose_matrix, local_matrix);
	mat_mul44(transform->matrix, parent->matrix, m);
}

static mat4 proj;
static mat4 view;

void render_camera(mat4 iproj, mat4 iview)
{
	mat_copy(proj, iproj);
	mat_copy(view, iview);
}

void render_skelpose(struct transform *transform, struct skelpose *skelpose)
{
	struct skel *skel = skelpose->skel;
	struct pose *pose = skelpose->pose;
	mat4 local_pose[MAXBONE];
	mat4 model_pose[MAXBONE];
	mat4 model_view;

	calc_matrix_from_pose(local_pose, pose, skel->count);
	calc_abs_matrix(model_pose, local_pose, skel->parent, skel->count);

	mat_mul(model_view, view, transform->matrix);

	draw_begin(proj, model_view);
	draw_set_color(1, 1, 1, 1);
	draw_skel(model_pose, skel->parent, skel->count);
	draw_end();
}

void animate_skelpose(struct skelpose *skelpose, struct anim *anim, float frame, float blend)
{
	struct skel *skel = skelpose->skel;
	struct pose *pose = skelpose->pose;
	struct skel *askel = anim->skel;
	struct pose apose[MAXBONE];
	int si, ai;

	extract_frame(apose, anim, frame);

	if (anim->loop) {
		vec3 dpos;
		vec4 drot, identity, tmp;
		float t;

		t = frame / (anim->frames - 1);
		vec_scale(dpos, anim->motion.position, t);
		vec_sub(apose[0].position, apose[0].position, dpos);

		quat_init(identity, 0, 0, 0, 1);
		quat_conjugate(drot, anim->motion.rotation);
		quat_lerp_neighbor_normalize(drot, identity, drot, t);
		quat_copy(tmp, apose[0].rotation);
		quat_mul(apose[0].rotation, drot, tmp);
	}

	for (si = 0; si < skel->count; si++) {
		// TODO: bone map
		ai = find_bone(askel, skel->name[si]);
		if (blend == 1)
			pose[si] = ai >= 0 ? apose[ai] : skel->pose[si];
		else
			pose_lerp(pose+si, pose+si, ai >= 0 ? apose+ai : skel->pose+si, blend);
	}
}

void render_mesh_skel(struct transform *transform, struct mesh *mesh, struct skelpose *skelpose)
{
	struct skel *skel = skelpose->skel;
	struct pose *pose = skelpose->pose;
	struct skel *ms = mesh->skel;
	mat4 local_pose[MAXBONE];
	mat4 model_pose[MAXBONE];
	mat4 model_view;
	mat4 model_from_bind_pose[MAXBONE];
	int mi, si;

	calc_matrix_from_pose(local_pose, pose, skel->count);
	calc_abs_matrix(model_pose, local_pose, skel->parent, skel->count);

	mat_mul(model_view, view, transform->matrix);

	for (mi = 0; mi < ms->count; mi++) {
		// TODO: bone map
		si = find_bone(skel, ms->name[mi]);
		if (si < 0) {
			fprintf(stderr, "cannot find bone: %s\n", skel->name[mi]);
			return; /* error! */
		}
		mat_mul(model_from_bind_pose[mi], model_pose[si], mesh->inv_bind_matrix[mi]);
	}

	render_skinned_mesh(mesh, proj, model_view, model_from_bind_pose);
}

void render_mesh(struct transform *transform, struct mesh *mesh)
{
	mat4 model_view;
	mat_mul(model_view, view, transform->matrix);
	render_static_mesh(mesh, proj, model_view);
}

void render_lamp(struct transform *transform, struct lamp *lamp)
{
	switch (lamp->type) {
	case LAMP_POINT: render_point_lamp(lamp, proj, view, transform->matrix); break;
	case LAMP_SPOT: render_spot_lamp(lamp, proj, view, transform->matrix); break;
	case LAMP_SUN: render_sun_lamp(lamp, proj, view, transform->matrix); break;
	}
}
