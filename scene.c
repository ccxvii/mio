#include "mio.h"

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	scene->tag = TAG_SCENE;
	LIST_INIT(&scene->armatures);
	LIST_INIT(&scene->objects);
	LIST_INIT(&scene->lamps);
	return scene;
}

struct armature *new_armature(struct scene *scene, struct skel *skel)
{
	struct armature *amt = malloc(sizeof(*amt));
	memset(amt, 0, sizeof(*amt));
	amt->tag = TAG_ARMATURE;
	amt->skel = skel;

	amt->dirty = 1;
	vec_init(amt->position, 0, 0, 0);
	quat_init(amt->rotation, 0, 0, 0, 1);
	vec_init(amt->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->armatures, amt, list);
	return amt;
}

struct object *new_object(struct scene *scene, struct mesh *mesh)
{
	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));
	obj->tag = TAG_OBJECT;

	obj->mesh = mesh;

	obj->dirty = 1;
	vec_init(obj->position, 0, 0, 0);
	quat_init(obj->rotation, 0, 0, 0, 1);
	vec_init(obj->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->objects, obj, list);
	return obj;
}

struct lamp *new_lamp(struct scene *scene)
{
	struct lamp *lamp = malloc(sizeof(*lamp));
	memset(lamp, 0, sizeof(*lamp));
	lamp->tag = TAG_LAMP;

	lamp->dirty = 1;
	vec_init(lamp->position, 0, 0, 0);
	quat_init(lamp->rotation, 0, 0, 0, 1);
	vec_init(lamp->scale, 1, 1, 1);

	lamp->type = LAMP_POINT;
	vec_init(lamp->color, 1, 1, 1);
	lamp->energy = 1;
	lamp->distance = 25;
	lamp->spot_angle = 45;

	LIST_INSERT_HEAD(&scene->lamps, lamp, list);
	return lamp;
}

int armature_set_parent(struct armature *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}

	return -1;
}

int object_set_parent(struct object *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	struct skel *mskel = node->mesh->skel;
	int i, k;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	if (mskel) {
		for (i = 0; i < mskel->count; i++) {
			for (k = 0; k < skel->count; k++) {
				if (!strcmp(mskel->name[i], skel->name[k])) {
					node->parent_map[i] = k;
					break;
				}
			}
			if (k == skel->count)
				return -1; /* bone missing from skeleton */
		}
		node->parent = parent;
		return 0;
	}

	else if (tagname) {
		for (i = 0; i < skel->count; i++) {
			if (!strcmp(skel->name[i], tagname)) {
				node->parent = parent;
				node->parent_tag = i;
				return 0;
			}
		}
		return -1; /* tag not found */
	}

	return -1; /* no tag, no skin */
}

int lamp_set_parent(struct lamp *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;

	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;

	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}

	return -1;
}

void armature_clear_parent(struct armature *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void object_clear_parent(struct object *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void lamp_clear_parent(struct lamp *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void play_anim(struct armature *node, struct anim *anim, float transition)
{
	struct skel *skel = node->skel;
	struct skel *askel = anim->skel;
	struct anim_map *map;

	for (map = anim->anim_map_head; map; map = map->next)
		if (map->skel == node->skel)
			break;

	if (!map) {
		int i, k;

		map = malloc(sizeof(*map));
		map->skel = node->skel;
		map->next = anim->anim_map_head;
		anim->anim_map_head = map;

		for (i = 0; i < skel->count; i++) {
			for (k = 0; k < askel->count; k++) {
				if (!strcmp(skel->name[i], askel->name[k])) {
					map->anim_map[i] = k;
					break;
				}
			}
			if (k == askel->count)
				map->anim_map[i] = -1; /* bone missing from skeleton */
		}
	}

	if (transition > 0) {
		node->oldanim = node->curanim;
		node->phase = 0;
		node->speed = transition;
	} else {
		node->phase = 1;
	}

	node->curanim.map = map->anim_map;
	node->curanim.anim = anim;
	node->curanim.frame = 0;

	node->dirty = 1;
}

void stop_anim(struct armature *node)
{
	node->curanim.map = NULL;
	node->curanim.anim = NULL;
	node->curanim.frame = 0;
	node->dirty = 1;
}

static void calc_root_motion(mat4 root_motion, struct anim *anim, float frame)
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

static void update_anim_play(struct anim_play *play, float delta)
{
	struct anim *anim = play->anim;
	play->frame += delta * anim->framerate;
	if (play->frame > anim->frames - 1) {
		if (anim->loop)
			play->frame -= anim->frames - 1;
		else
			play->frame = anim->frames - 1;
	}
}

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
	mat4 *pose_matrix = node->parent->model_pose;
	mat4 *inv_bind_matrix = node->mesh->inv_bind_matrix;
	mat4 *model_from_bind_pose = node->model_from_bind_pose;
	unsigned char *parent_map = node->parent_map;
	int i, count = node->mesh->skel->count;
	for (i = 0; i < count; i++)
		mat_mul(model_from_bind_pose[i], pose_matrix[parent_map[i]], inv_bind_matrix[i]);
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

void update_scene(struct scene *scene, float delta)
{
	struct object *obj;
	struct armature *amt;
	struct lamp *lamp;

	LIST_FOREACH(amt, &scene->armatures, list)
		amt->updated = 0;

	LIST_FOREACH(amt, &scene->armatures, list)
		update_armature(amt, delta);
	LIST_FOREACH(obj, &scene->objects, list)
		update_object(obj, delta);
	LIST_FOREACH(lamp, &scene->lamps, list)
		update_lamp(lamp, delta);

	LIST_FOREACH(amt, &scene->armatures, list)
		amt->dirty = 0;
}

void draw_armature(struct scene *scene, struct armature *node, mat4 proj, mat4 view)
{
	mat4 model_view;

	mat_mul(model_view, view, node->transform);

	draw_begin(proj, model_view);
	draw_set_color(1, 1, 1, 1);
	draw_skel(node->model_pose, node->skel->parent, node->skel->count);
	draw_end();
}

void render_object(struct scene *scene, struct object *node, mat4 proj, mat4 view)
{
	mat4 model_view;

	mat_mul(model_view, view, node->transform);

	if (node->mesh->skel && node->parent)
		render_skinned_mesh(node->mesh, proj, model_view, node->model_from_bind_pose);
	else
		render_static_mesh(node->mesh, proj, model_view);
}

void render_scene_geometry(struct scene *scene, mat4 proj, mat4 view)
{
	struct object *obj;
	LIST_FOREACH(obj, &scene->objects, list)
		render_object(scene, obj, proj, view);
}

void render_scene_light(struct scene *scene, mat4 proj, mat4 view)
{
	struct lamp *lamp;
	LIST_FOREACH(lamp, &scene->lamps, list) {
		switch (lamp->type) {
		case LAMP_POINT: render_point_lamp(lamp, proj, view); break;
		case LAMP_SPOT: render_spot_lamp(lamp, proj, view); break;
		case LAMP_SUN: render_sun_lamp(lamp, proj, view); break;
		}
	}
}

void draw_scene_debug(struct scene *scene, mat4 proj, mat4 view)
{
	struct armature *amt;

	glDisable(GL_DEPTH_TEST);
	LIST_FOREACH(amt, &scene->armatures, list) {
		draw_armature(scene, amt, proj, view);
	}
	glEnable(GL_DEPTH_TEST);
}
