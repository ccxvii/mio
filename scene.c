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

	vec_init(skelpose->delta.position, 0, 0, 0);
	quat_init(skelpose->delta.rotation, 0, 0, 0, 1);
	vec_init(skelpose->delta.scale, 1, 1, 1);
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

void vec_delta(vec3 p, const vec3 a, const vec3 b, float t)
{
	p[0] = t * (b[0] - a[0]);
	p[1] = t * (b[1] - a[1]);
	p[2] = t * (b[2] - a[2]);
}

void quat_delta(vec4 p, const vec4 a, const vec4 b, float t)
{
	p[0] = t * (b[0] - a[0]);
	p[1] = t * (b[1] - a[1]);
	p[2] = t * (b[2] - a[2]);
	p[3] = t * (b[3] - a[3]);
}

void quat_delta_neighbor(vec4 p, const vec4 a, const vec4 b, float t)
{
	if (a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3] < 0) {
		vec4 temp = { -a[0], -a[1], -a[2], -a[3] };
		quat_delta(p, temp, b, t);
	} else {
		quat_delta(p, a, b, t);
	}
}

void calc_root_delta(struct pose *p, struct anim *anim)
{
	struct pose p0, p1;

	float t = 1.0 / (anim->frames - 1);
	extract_frame_root(&p0, anim, 0);
	extract_frame_root(&p1, anim, anim->frames - 1);

	vec_delta(p->position, p0.position, p1.position, t);
	quat_delta_neighbor(p->rotation, p0.rotation, p1.rotation, t);
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

void update_transform_root_motion(struct transform *transform, struct skelpose *skelpose)
{
	vec_add(transform->position, transform->position, skelpose->delta.position);
}

void animate_skelpose(struct skelpose *skelpose, struct anim *anim, float frame, float blend)
{
	struct skel *skel = skelpose->skel;
	struct pose *pose = skelpose->pose;
	struct skel *askel = anim->skel;
	struct pose apose[MAXBONE];
	struct pose delta;
	int si, ai;

	extract_frame(apose, anim, frame);

	calc_root_delta(&delta, anim); // TODO: calc once and save in anim

	if (blend == 1)
		skelpose->delta = delta;
	else
		pose_lerp(&skelpose->delta, &skelpose->delta, &delta, blend);

	vec_scale(delta.position, delta.position, frame);
	vec_sub(apose[0].position, apose[0].position, delta.position);

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
