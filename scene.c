#include "mio.h"

struct component
{
	enum tag tag;
};

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	scene->tag = TAG_SCENE;
	LIST_INIT(&scene->entity_list);
	return scene;
}

struct entity *new_entity(struct scene *scn)
{
	struct entity *ent = malloc(sizeof(*ent));
	memset(ent, 0, sizeof(*ent));
	ent->tag = TAG_ENTITY;
	LIST_INSERT_HEAD(&scn->entity_list, ent, entity_next);
	return ent;
}

void *new_component(int size, int tag)
{
	struct component *com = malloc(size);
	memset(com, 0, size);
	com->tag = tag;
	return com;
}

struct transform *new_transform(void)
{
	struct transform *tra = new_component(sizeof(*tra), TAG_TRANSFORM);
	tra->parent = NULL;
	tra->parent_bone = -1;
	vec_init(tra->pose.position, 0, 0, 0);
	quat_init(tra->pose.rotation, 0, 0, 0, 1);
	vec_init(tra->pose.scale, 1, 1, 1);
	mat_identity(tra->matrix);
	return tra;
}

struct iskel *new_iskel(struct skel *skel)
{
	struct iskel *iskel = new_component(sizeof(*iskel), TAG_ISKEL);
	iskel->skel = skel;
	return iskel;
}

struct imesh *new_imesh(struct mesh *mesh)
{
	struct imesh *imesh = new_component(sizeof(*imesh), TAG_IMESH);
	imesh->mesh = mesh;
	return imesh;
}

struct lamp *new_lamp(void)
{
	struct lamp *ilamp = new_component(sizeof(*ilamp), TAG_LAMP);
	ilamp->type = LAMP_POINT;
	vec_init(ilamp->color, 1, 1, 1);
	ilamp->energy = 1;
	ilamp->distance = 25;
	ilamp->spot_angle = 45;
	return ilamp;
}

int transform_set_parent(struct transform *tra, struct entity *parent, const char *bone_name)
{
	int i;

	if (parent->iskel && bone_name) {
		struct skel *skel = parent->iskel->skel;
		for (i = 0; i < skel->count; i++) {
			if (!strcmp(skel->name[i], bone_name)) {
				tra->parent = parent;
				tra->parent_bone = i;
				tra->dirty = 1;
				return 0;
			}
		}
	}

	return -1;
}

void transform_clear_parent(struct transform *tra)
{
	tra->parent = NULL;
	tra->parent_bone = 0;
	tra->dirty = 1;
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

#if 0
static int update_armature(struct armature *node, float delta)
{
	if (node->updated)
		return node->dirty;
	node->updated = 1;

	node->phase += delta;
	if (node->phase >= 1) {
		node->oldanim.anim = NULL;
	}
	if (node->curanim.anim) {
		update_anim_play(&node->curanim, delta);
		node->dirty = 1;
	}
	if (node->oldanim.anim) {
		update_anim_play(&node->oldanim, delta);
		node->dirty = 1;
	}

	if (node->parent)
		node->dirty |= update_armature(node->parent, delta);

	if (node->dirty) {
		mat4 local_pose[MAXBONE];

		if (node->parent) {
			mat4 parent, local;
			mat_from_pose(local, node->position, node->rotation, node->scale);
			mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
			mat_mul(node->transform, parent, local);
		} else {
			mat_from_pose(node->transform, node->position, node->rotation, node->scale);
		}

		if (node->curanim.anim) {
			struct pose *skel_pose = node->skel->pose;
			struct pose anim_pose0[MAXBONE];
			struct pose anim_pose1[MAXBONE];
			int i;

			if (node->oldanim.anim) {
				extract_frame(anim_pose0, node->oldanim.anim, node->oldanim.frame);
				extract_frame(anim_pose1, node->curanim.anim, node->curanim.frame);
				for (i = 0; i < node->skel->count; i++) {
					int k0 = node->oldanim.map[i];
					int k1 = node->curanim.map[i];
					struct pose *p0 = k0 >= 0 ? anim_pose0 + k0 : skel_pose + i;
					struct pose *p1 = k1 >= 0 ? anim_pose1 + k1 : skel_pose + i;
					struct pose p;
					lerp_frame(&p, p0, p1, node->phase, 1);
					mat_from_pose(local_pose[i], p.position, p.rotation, p.scale);
				}
			} else {
				extract_frame(anim_pose1, node->curanim.anim, node->curanim.frame);
				for (i = 0; i < node->skel->count; i++) {
					int k = node->curanim.map[i];
					if (k >= 0)
						mat_from_pose(local_pose[i], anim_pose1[k].position, anim_pose1[k].rotation, anim_pose1[k].scale);
					else
						mat_from_pose(local_pose[i], skel_pose[i].position, skel_pose[i].rotation, skel_pose[i].scale);
				}
			}

			calc_abs_matrix(node->model_pose, local_pose, node->skel->parent, node->skel->count);

			// TODO: blending
			mat4 root_motion;
			calc_root_motion(root_motion, node->curanim.anim, node->curanim.frame);
			mat_mul(node->transform, node->transform, root_motion);
		} else {
			calc_matrix_from_pose(local_pose, node->skel->pose, node->skel->count);
			calc_abs_matrix(node->model_pose, local_pose, node->skel->parent, node->skel->count);
		}
	}

	return node->dirty;
}

static void update_object_skin(struct object *node)
{
}

static void update_object(struct object *node, float delta)
{
	if (node->parent)
		node->dirty |= update_armature(node->parent, delta);

	if (node->dirty) {
		if (node->parent) {
			if (node->mesh->skel) {
				if (!node->model_from_bind_pose)
					node->model_from_bind_pose = malloc(node->mesh->skel->count * sizeof(mat4));
				update_object_skin(node);
				mat_copy(node->transform, node->parent->transform);
			} else {
				mat4 parent, local;
				mat_from_pose(local, node->position, node->rotation, node->scale);
				mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
				mat_mul(node->transform, parent, local);
			}
		} else {
			mat_from_pose(node->transform, node->position, node->rotation, node->scale);
		}
	}

	node->dirty = 0;
}

static void update_lamp(struct lamp *node, float delta)
{
	if (node->parent)
		node->dirty |= update_armature(node->parent, delta);

	if (node->dirty) {
		if (node->parent) {
			mat4 parent, local;
			mat_from_pose(local, node->position, node->rotation, node->scale);
			mat_mul(parent, node->parent->transform, node->parent->model_pose[node->parent_tag]);
			mat_mul(node->transform, parent, local);
		} else {
			mat_from_pose(node->transform, node->position, node->rotation, node->scale);
		}
	}

	node->dirty = 0;
}
#endif

void calc_iskel(struct iskel *iskel)
{
	mat4 local_matrix[MAXBONE];
	calc_matrix_from_pose(local_matrix, iskel->pose, iskel->skel->count);
	calc_abs_matrix(iskel->matrix, local_matrix, iskel->skel->parent, iskel->skel->count);
}

void calc_imesh(struct imesh *imesh, struct iskel *iskel)
{
	mat4 *pose_matrix = iskel->matrix;
	mat4 *inv_bind_matrix = imesh->mesh->inv_bind_matrix;
	mat4 *model_from_bind_pose = imesh->model_from_bind_pose;
	unsigned char *skel_map = imesh->skel_map; // map iskel->skel -> imesh->mesh->skel
	int i, count = imesh->mesh->skel->count;
	for (i = 0; i < count; i++)
		mat_mul(model_from_bind_pose[i], pose_matrix[skel_map[i]], inv_bind_matrix[i]);
}

void calc_transform(struct transform *transform)
{
	if (transform->parent) {
		struct entity *parent = transform->parent;
		mat4 local_mat;
		calc_transform(parent->transform);
		mat_from_pose(local_mat, transform->pose.position, transform->pose.rotation, transform->pose.scale);
		if (transform->parent_bone) {
			mat4 parent_mat;
			calc_iskel(parent->iskel);
			mat_mul(parent_mat, parent->transform->matrix, parent->iskel->matrix[transform->parent_bone]);
			mat_mul(transform->matrix, parent_mat, local_mat);
		} else {
			mat_mul(transform->matrix, parent->transform->matrix, local_mat);
		}
	} else {
		mat_from_pose(transform->matrix, transform->pose.position, transform->pose.rotation, transform->pose.scale);
	}
}

void draw_iskel(struct iskel *iskel, mat4 proj, mat4 view, struct transform *transform)
{
	mat4 model_view;

	calc_transform(transform);
	calc_iskel(iskel);

	mat_mul(model_view, view, transform->matrix);

	draw_begin(proj, model_view);
	draw_set_color(1, 1, 1, 1);
	draw_skel(iskel->matrix, iskel->skel->parent, iskel->skel->count);
	draw_end();
}

void render_mesh(struct imesh *imesh, struct iskel *iskel, mat4 proj, mat4 view, struct transform *transform)
{
	mat4 model_view;

	calc_transform(transform);

	mat_mul(model_view, view, transform->matrix);

	if (iskel) {
		calc_iskel(iskel);
		calc_imesh(imesh, iskel);
		render_skinned_mesh(imesh->mesh, proj, model_view, imesh->model_from_bind_pose);
	}
	else {
		render_static_mesh(imesh->mesh, proj, model_view);
	}
}

void render_scene_geometry(struct scene *scene, mat4 proj, mat4 view)
{
	struct entity *ent;
	struct imesh *imesh;
	LIST_FOREACH(ent, &scene->entity_list, entity_next) {
		LIST_FOREACH(imesh, &ent->imesh_list, imesh_next) {
			render_mesh(imesh, ent->iskel, proj, view, ent->transform);
		}
	}
}

void render_scene_light(struct scene *scene, mat4 proj, mat4 view)
{
	struct entity *ent;
	LIST_FOREACH(ent, &scene->entity_list, entity_next) {
		if (ent->lamp) {
			calc_transform(ent->transform);
			switch (ent->lamp->type) {
			case LAMP_POINT: render_point_lamp(ent->lamp, proj, view, ent->transform->matrix); break;
			case LAMP_SPOT: render_spot_lamp(ent->lamp, proj, view, ent->transform->matrix); break;
			case LAMP_SUN: render_sun_lamp(ent->lamp, proj, view, ent->transform->matrix); break;
			}
		}
	}
}

void draw_scene_debug(struct scene *scene, mat4 proj, mat4 view)
{
	glDisable(GL_DEPTH_TEST);
	struct entity *ent;
	LIST_FOREACH(ent, &scene->entity_list, entity_next) {
		if (ent->iskel)
			draw_iskel(ent->iskel, proj, view, ent->transform);
	}
	glEnable(GL_DEPTH_TEST);
}
