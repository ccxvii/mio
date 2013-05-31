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
	vec_init(amt->location, 0, 0, 0);
	quat_init(amt->rotation, 0, 0, 0, 1);
	vec_init(amt->scale, 1, 1, 1);
	amt->dirty = 1;
	LIST_INSERT_HEAD(&scene->armatures, amt, list);
	return amt;
}

struct object *new_object(struct scene *scene, const char *meshname)
{
	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));
	obj->tag = TAG_OBJECT;
	obj->mesh = load_mesh(meshname);
	vec_init(obj->location, 0, 0, 0);
	quat_init(obj->rotation, 0, 0, 0, 1);
	vec_init(obj->scale, 1, 1, 1);
	obj->dirty = 1;
	LIST_INSERT_HEAD(&scene->objects, obj, list);
	return obj;
}

struct light *new_light(struct scene *scene)
{
	struct light *light = malloc(sizeof(*light));
	memset(light, 0, sizeof(*light));
	light->tag = TAG_LIGHT;
	vec_init(light->location, 0, 0, 0);
	quat_init(light->rotation, 0, 0, 0, 1);
	vec_init(light->scale, 1, 1, 1);
	light->dirty = 1;
	LIST_INSERT_HEAD(&scene->lights, light, list);
	return light;
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
