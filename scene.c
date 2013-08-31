#include "mio.h"

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

void calc_root_motion(mat4 root_motion, struct anim *anim, float frame)
{
	struct pose p0, p1, p;
	mat4 m0, m;
	float t = frame / (anim->frames - 1);

	extract_frame_root(&p0, anim, 0);
	extract_frame_root(&p1, anim, anim->frames - 1);

	vec_lerp(p.position, p0.position, p1.position, t);
	quat_lerp_neighbor_normalize(p.rotation, p0.rotation, p1.rotation, t);
	vec_lerp(p.scale, p0.scale, p1.scale, t);

	mat_from_pose(m0, p0.position, p0.rotation, p0.scale);
	mat_from_pose(m, p.position, p.rotation, p.scale);

	mat_invert(m, m);
	mat_mul(root_motion, m0, m);
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

	mat_from_pose(transform->matrix, transform->position, transform->rotation, transform->scale);

	calc_matrix_from_pose(local_pose, pose, skel->count);
	calc_abs_matrix(model_pose, local_pose, skel->parent, skel->count);

	mat_mul(model_view, view, transform->matrix);

	draw_begin(proj, model_view);
	draw_set_color(1, 1, 1, 1);
	draw_skel(model_pose, skel->parent, skel->count);
	draw_end();
}

static int find_bone(struct skel *skel, const char *name)
{
	int i;
	for (i = 0; i < skel->count; i++)
		if (!strcmp(skel->name[i], name))
			return i;
	return -1;
}

void animate_skelpose(struct skelpose *skelpose, struct anim *anim, float frame)
{
	struct skel *skel = skelpose->skel;
	struct pose *pose = skelpose->pose;
	struct skel *askel = anim->skel;
	struct pose apose[MAXBONE];
	int si, ai;

	extract_frame(apose, anim, frame);
	for (si = 0; si < skel->count; si++) {
		ai = find_bone(askel, skel->name[si]);
		if (ai >= 0)
			pose[si] = apose[ai];
		else
			pose[si] = skel->pose[si];
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

	mat_from_pose(transform->matrix, transform->position, transform->rotation, transform->scale);

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

	mat_from_pose(transform->matrix, transform->position, transform->rotation, transform->scale);

	mat_mul(model_view, view, transform->matrix);
	render_static_mesh(mesh, proj, model_view);
}

void render_lamp(struct transform *transform, struct lamp *lamp)
{
	mat_from_pose(transform->matrix, transform->position, transform->rotation, transform->scale);
	switch (lamp->type) {
	case LAMP_POINT: render_point_lamp(lamp, proj, view, transform->matrix); break;
	case LAMP_SPOT: render_spot_lamp(lamp, proj, view, transform->matrix); break;
	case LAMP_SUN: render_sun_lamp(lamp, proj, view, transform->matrix); break;
	}
}
