#include "mio.h"

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	scene->tag = TAG_SCENE;
	LIST_INIT(&scene->armatures);
	LIST_INIT(&scene->objects);
	LIST_INIT(&scene->lights);
	return scene;
}

struct armature *new_armature(struct scene *scene, const char *skelname)
{
	struct armature *amt = malloc(sizeof(*amt));
	memset(amt, 0, sizeof(*amt));
	amt->tag = TAG_ARMATURE;
	amt->skel = load_skel(skelname);

	amt->dirty = 1;
	vec_init(amt->location, 0, 0, 0);
	quat_init(amt->rotation, 0, 0, 0, 1);
	vec_init(amt->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->armatures, amt, list);
	return amt;
}

struct object *new_object(struct scene *scene, const char *meshname)
{
	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));
	obj->tag = TAG_OBJECT;

	obj->mesh = load_mesh(meshname);

	obj->dirty = 1;
	vec_init(obj->location, 0, 0, 0);
	quat_init(obj->rotation, 0, 0, 0, 1);
	vec_init(obj->scale, 1, 1, 1);

	LIST_INSERT_HEAD(&scene->objects, obj, list);
	return obj;
}

struct light *new_light(struct scene *scene)
{
	struct light *light = malloc(sizeof(*light));
	memset(light, 0, sizeof(*light));
	light->tag = TAG_LIGHT;

	light->dirty = 1;
	vec_init(light->location, 0, 0, 0);
	quat_init(light->rotation, 0, 0, 0, 1);
	vec_init(light->scale, 1, 1, 1);

	light->type = LIGHT_POINT;
	vec_init(light->color, 1, 1, 1);
	light->energy = 1;
	light->distance = 25;
	light->spot_angle = 45;

	LIST_INSERT_HEAD(&scene->lights, light, list);
	return light;
}

int attach_armature(struct armature *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;
	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}
	return -1;
}

int attach_object(struct object *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	struct skel *mskel = node->mesh->skel;
	int i, k;

	if (tagname) {
		for (i = 0; i < skel->count; i++) {
			if (!strcmp(skel->name[i], tagname)) {
				node->parent = parent;
				node->parent_tag = i;
				return 0;
			}
		}
		return -1;
	}

	else if (mskel) {
		node->parent = parent;
		node->parent_tag = 0;
		for (i = 0; i < mskel->count; i++) {
			for (k = 0; k < skel->count; k++) {
				if (!strcmp(mskel->name[i], skel->name[i])) {
					node->parent_map[i] = k;
					break;
				}
			}
		}
		return 0;
	}

	return -1;
}

int attach_light(struct light *node, struct armature *parent, const char *tagname)
{
	struct skel *skel = parent->skel;
	int i;
	for (i = 0; i < skel->count; i++) {
		if (!strcmp(skel->name[i], tagname)) {
			node->parent = parent;
			node->parent_tag = i;
			return 0;
		}
	}
	return -1;
}

void detach_armature(struct armature *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void detach_object(struct object *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void detach_light(struct light *node)
{
	node->dirty = 1;
	node->parent = NULL;
	node->parent_tag = 0;
}

void draw_object(struct scene *scene, struct object *obj, mat4 projection, mat4 view)
{
	mat4 model_view;
	if (obj->dirty) {
		mat_from_pose(obj->transform, obj->location, obj->rotation, obj->scale);
		obj->dirty = 0;
	}
	mat_mul(model_view, view, obj->transform);
	draw_model(obj->mesh, projection, model_view);
}

void draw_scene(struct scene *scene, mat4 projection, mat4 view)
{
	struct object *obj;

	LIST_FOREACH(obj, &scene->objects, list) {
		draw_object(scene, obj, projection, view);
	}
}
