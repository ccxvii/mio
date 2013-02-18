#include "mio.h"

struct scene *new_scene(void)
{
	struct scene *scene = malloc(sizeof(*scene));
	LIST_INIT(&scene->armatures);
	LIST_INIT(&scene->objects);
	LIST_INIT(&scene->lights);
	return scene;
}

struct armature *new_armature(struct scene *scene, const char *skelname)
{
	struct armature *amt = malloc(sizeof(*amt));
	memset(amt, 0, sizeof(*amt));

	amt->skel = load_skel(skelname);
	mat_identity(amt->transform);

	LIST_INSERT_HEAD(&scene->armatures, amt, list);
	return amt;
}

struct object *new_object(struct scene *scene, const char *meshname)
{
	struct object *obj = malloc(sizeof(*obj));
	memset(obj, 0, sizeof(*obj));

	obj->mesh = load_mesh(meshname);
	mat_identity(obj->transform);

	LIST_INSERT_HEAD(&scene->objects, obj, list);
	return obj;
}

struct light *new_light(struct scene *scene)
{
	struct light *light = malloc(sizeof(*light));
	memset(light, 0, sizeof(*light));

	LIST_INSERT_HEAD(&scene->lights, light, list);
	return light;
}

void draw_object(struct scene *scene, struct object *obj, mat4 projection, mat4 view)
{
	mat4 model_view;
	mat_mul(model_view, view, obj->transform);
	draw_model(obj->mesh, projection, model_view);
}

void draw_scene(struct scene *scene, mat4 projection, mat4 view)
{
	struct armature *amt;
	struct object *obj;

	LIST_FOREACH(obj, &scene->objects, list) {
		draw_object(scene, obj, projection, view);
	}
}
