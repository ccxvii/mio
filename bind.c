#include "mio.h"

struct scene *scene = NULL;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *L = NULL;

static int ffi_print(lua_State *L)
{
	int i, n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1); /* tostring */
		lua_pushvalue(L, i); /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1); /* get result */
		if (!s) return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) console_print("\t");
		console_print(s);
		lua_pop(L, 1); /* pop result */
	}
	console_putc('\n');
	return 0;
}

static int ffi_traceback(lua_State *L)
{
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

void run_string(const char *cmd)
{
	if (!L) {
		console_print("[no command interpreter]\n");
		return;
	}

	int status = luaL_loadstring(L, cmd);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printf("%s\n", msg);
		lua_pop(L, 1);
		return;
	}

	lua_pushcfunction(L, ffi_traceback);
	lua_insert(L, 1);
	status = lua_pcall(L, 0, LUA_MULTRET, 1);
	lua_remove(L, 1);

	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printf("%s\n", msg);
		lua_pop(L, 1);
		return;
	}

	if (lua_gettop(L) > 0) {
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		lua_pcall(L, lua_gettop(L)-1, 0, 0);
	}
}

void run_file(const char *filename)
{
	if (!L) {
		console_print("[no command interpreter]\n");
		return;
	}

	int status = luaL_loadfile(L, filename);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printf("%s\n", msg);
		lua_pop(L, 1);
		return;
	}

	lua_pushcfunction(L, ffi_traceback);
	lua_insert(L, 1);
	status = lua_pcall(L, 0, 0, 1);
	lua_remove(L, 1);

	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printf("%s\n", msg);
		lua_pop(L, 1);
		return;
	}
}

static void *checktag(lua_State *L, int n, int tag)
{
	luaL_checktype(L, n, LUA_TLIGHTUSERDATA);
	int *p = lua_touserdata(L, n);
	if (!p || *p != tag)
		luaL_argerror(L, n, "wrong userdata type");
	return p;
}

/* Armature */

static int ffi_amt_new(lua_State *L)
{
	const char *skelname = luaL_checkstring(L, 1);
	struct armature *amt = new_armature(scene, skelname);
	if (!amt)
		return luaL_error(L, "cannot load skeleton: %s", skelname);
	lua_pushlightuserdata(L, amt);
	return 1;
}

static int ffi_amt_set_parent(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = luaL_checkstring(L, 3);
	if (armature_set_parent(amt, parent, tagname))
		return luaL_error(L, "cannot find bone: %s", tagname);
	return 0;
}

static int ffi_amt_clear_parent(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	armature_clear_parent(amt);
	return 0;
}

static int ffi_amt_set_location(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->location[0] = luaL_checknumber(L, 2);
	amt->location[1] = luaL_checknumber(L, 3);
	amt->location[2] = luaL_checknumber(L, 4);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_rotation(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->rotation[0] = luaL_checknumber(L, 2);
	amt->rotation[1] = luaL_checknumber(L, 3);
	amt->rotation[2] = luaL_checknumber(L, 4);
	amt->rotation[3] = luaL_checknumber(L, 5);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_scale(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->scale[0] = luaL_checknumber(L, 2);
	amt->scale[1] = luaL_checknumber(L, 3);
	amt->scale[2] = luaL_checknumber(L, 4);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_location(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->location[0]);
	lua_pushnumber(L, amt->location[1]);
	lua_pushnumber(L, amt->location[2]);
	return 3;
}

static int ffi_amt_rotation(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->rotation[0]);
	lua_pushnumber(L, amt->rotation[1]);
	lua_pushnumber(L, amt->rotation[2]);
	lua_pushnumber(L, amt->rotation[3]);
	return 4;
}

static int ffi_amt_scale(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	lua_pushnumber(L, amt->scale[0]);
	lua_pushnumber(L, amt->scale[1]);
	lua_pushnumber(L, amt->scale[2]);
	return 3;
}

static int ffi_amt_play_anim(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	const char *animname = luaL_checkstring(L, 2);
	float time = luaL_checknumber(L, 3);
	struct anim *anim = load_anim(animname);
	if (!anim)
		return luaL_error(L, "cannot load anim: %s", animname);
	play_anim(amt, anim, time);
	return 0;
}

static int ffi_amt_stop_anim(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	stop_anim(amt);
	return 0;
}

/* Object */

static int ffi_obj_new(lua_State *L)
{
	const char *meshname = luaL_checkstring(L, 1);
	struct object *obj = new_object(scene, meshname);
	if (!obj)
		return luaL_error(L, "cannot load mesh: %s", meshname);
	lua_pushlightuserdata(L, obj);
	return 1;
}

static int ffi_obj_set_parent(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = lua_tostring(L, 3);
	if (object_set_parent(obj, parent, tagname)) {
		if (tagname)
			return luaL_error(L, "cannot find bone: %s", tagname);
		else
			return luaL_error(L, "skeleton mismatch");
	}
	return 0;
}

static int ffi_obj_clear_parent(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	object_clear_parent(obj);
	return 0;
}

static int ffi_obj_set_location(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->location[0] = luaL_checknumber(L, 2);
	obj->location[1] = luaL_checknumber(L, 3);
	obj->location[2] = luaL_checknumber(L, 4);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_rotation(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->rotation[0] = luaL_checknumber(L, 2);
	obj->rotation[1] = luaL_checknumber(L, 3);
	obj->rotation[2] = luaL_checknumber(L, 4);
	obj->rotation[3] = luaL_checknumber(L, 5);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_scale(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->scale[0] = luaL_checknumber(L, 2);
	obj->scale[1] = luaL_checknumber(L, 3);
	obj->scale[2] = luaL_checknumber(L, 4);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_color(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->color[0] = luaL_checknumber(L, 2);
	obj->color[1] = luaL_checknumber(L, 3);
	obj->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_obj_location(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->location[0]);
	lua_pushnumber(L, obj->location[1]);
	lua_pushnumber(L, obj->location[2]);
	return 3;
}

static int ffi_obj_rotation(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->rotation[0]);
	lua_pushnumber(L, obj->rotation[1]);
	lua_pushnumber(L, obj->rotation[2]);
	lua_pushnumber(L, obj->rotation[3]);
	return 4;
}

static int ffi_obj_scale(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->scale[0]);
	lua_pushnumber(L, obj->scale[1]);
	lua_pushnumber(L, obj->scale[2]);
	return 3;
}

static int ffi_obj_color(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	lua_pushnumber(L, obj->color[0]);
	lua_pushnumber(L, obj->color[1]);
	lua_pushnumber(L, obj->color[2]);
	return 3;
}

/* Light */

static const char *light_type_enum[] = { "POINT", "SPOT", "SUN", 0 };

static int ffi_light_new(lua_State *L)
{
	struct light *light = new_light(scene);
	if (!light)
		return luaL_error(L, "cannot create light");
	lua_pushlightuserdata(L, light);
	return 1;
}

static int ffi_light_set_parent(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	struct armature *parent = checktag(L, 2, TAG_ARMATURE);
	const char *tagname = luaL_checkstring(L, 3);
	if (light_set_parent(light, parent, tagname))
		return luaL_error(L, "cannot find bone: %s", tagname);
	return 0;
}

static int ffi_light_clear_parent(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light_clear_parent(light);
	return 0;
}

static int ffi_light_set_location(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->location[0] = luaL_checknumber(L, 2);
	light->location[1] = luaL_checknumber(L, 3);
	light->location[2] = luaL_checknumber(L, 4);
	light->dirty = 1;
	return 0;
}

static int ffi_light_set_rotation(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->rotation[0] = luaL_checknumber(L, 2);
	light->rotation[1] = luaL_checknumber(L, 3);
	light->rotation[2] = luaL_checknumber(L, 4);
	light->rotation[3] = luaL_checknumber(L, 5);
	light->dirty = 1;
	return 0;
}

static int ffi_light_set_color(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->color[0] = luaL_checknumber(L, 2);
	light->color[1] = luaL_checknumber(L, 3);
	light->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_light_set_type(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->type = luaL_checkoption(L, 2, NULL, light_type_enum);
	return 0;
}

static int ffi_light_set_energy(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->energy = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_light_set_distance(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->distance = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_light_set_spot_angle(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->spot_angle = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_light_set_spot_blend(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->spot_blend = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_light_set_use_sphere(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->use_sphere = luaL_checkint(L, 2);
	return 0;
}

static int ffi_light_set_use_square(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->use_square = luaL_checkint(L, 2);
	return 0;
}

static int ffi_light_set_use_shadow(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->use_shadow = luaL_checkint(L, 2);
	return 0;
}

static int ffi_light_location(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->location[0]);
	lua_pushnumber(L, light->location[1]);
	lua_pushnumber(L, light->location[2]);
	return 3;
}

static int ffi_light_rotation(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->rotation[0]);
	lua_pushnumber(L, light->rotation[1]);
	lua_pushnumber(L, light->rotation[2]);
	lua_pushnumber(L, light->rotation[3]);
	return 4;
}

static int ffi_light_color(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->color[0]);
	lua_pushnumber(L, light->color[1]);
	lua_pushnumber(L, light->color[2]);
	return 3;
}

static int ffi_light_type(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushstring(L, light_type_enum[light->type]);
	return 1;
}

static int ffi_light_energy(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->energy);
	return 1;
}

static int ffi_light_distance(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->distance);
	return 1;
}

static int ffi_light_spot_angle(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->spot_angle);
	return 1;
}

static int ffi_light_spot_blend(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushnumber(L, light->spot_blend);
	return 1;
}

static int ffi_light_use_sphere(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushboolean(L, light->use_sphere);
	return 1;
}

static int ffi_light_use_square(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushboolean(L, light->use_square);
	return 1;
}

static int ffi_light_use_shadow(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	lua_pushboolean(L, light->use_shadow);
	return 1;
}

/* Misc */

static int ffi_register_archive(lua_State *L)
{
	register_archive(luaL_checkstring(L, 1));
	return 0;
}

static int ffi_register_directory(lua_State *L)
{
	register_directory(luaL_checkstring(L, 1));
	return 0;
}

void init_lua(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "print", ffi_print);

	lua_register(L, "register_archive", ffi_register_archive);
	lua_register(L, "register_directory", ffi_register_directory);

	lua_register(L, "amt_new", ffi_amt_new);
	lua_register(L, "amt_set_parent", ffi_amt_set_parent);
	lua_register(L, "amt_clear_parent", ffi_amt_clear_parent);
	lua_register(L, "amt_set_location", ffi_amt_set_location);
	lua_register(L, "amt_set_rotation", ffi_amt_set_rotation);
	lua_register(L, "amt_set_scale", ffi_amt_set_scale);
	lua_register(L, "amt_location", ffi_amt_location);
	lua_register(L, "amt_rotation", ffi_amt_rotation);
	lua_register(L, "amt_scale", ffi_amt_scale);
	lua_register(L, "amt_play_anim", ffi_amt_play_anim);
	lua_register(L, "amt_stop_anim", ffi_amt_stop_anim);

	lua_register(L, "obj_new", ffi_obj_new);
	lua_register(L, "obj_set_parent", ffi_obj_set_parent);
	lua_register(L, "obj_clear_parent", ffi_obj_clear_parent);
	lua_register(L, "obj_set_location", ffi_obj_set_location);
	lua_register(L, "obj_set_rotation", ffi_obj_set_rotation);
	lua_register(L, "obj_set_scale", ffi_obj_set_scale);
	lua_register(L, "obj_set_color", ffi_obj_set_color);
	lua_register(L, "obj_location", ffi_obj_location);
	lua_register(L, "obj_rotation", ffi_obj_rotation);
	lua_register(L, "obj_scale", ffi_obj_scale);
	lua_register(L, "obj_color", ffi_obj_color);

	lua_register(L, "light_new", ffi_light_new);
	lua_register(L, "light_set_parent", ffi_light_set_parent);
	lua_register(L, "light_clear_parent", ffi_light_clear_parent);
	lua_register(L, "light_set_location", ffi_light_set_location);
	lua_register(L, "light_set_rotation", ffi_light_set_rotation);
	lua_register(L, "light_set_type", ffi_light_set_type);
	lua_register(L, "light_set_color", ffi_light_set_color);
	lua_register(L, "light_set_energy", ffi_light_set_energy);
	lua_register(L, "light_set_distance", ffi_light_set_distance);
	lua_register(L, "light_set_spot_angle", ffi_light_set_spot_angle);
	lua_register(L, "light_set_spot_blend", ffi_light_set_spot_blend);
	lua_register(L, "light_set_use_sphere", ffi_light_set_use_sphere);
	lua_register(L, "light_set_use_square", ffi_light_set_use_square);
	lua_register(L, "light_set_use_shadow", ffi_light_set_use_shadow);
	lua_register(L, "light_location", ffi_light_location);
	lua_register(L, "light_rotation", ffi_light_rotation);
	lua_register(L, "light_type", ffi_light_type);
	lua_register(L, "light_color", ffi_light_color);
	lua_register(L, "light_energy", ffi_light_energy);
	lua_register(L, "light_distance", ffi_light_distance);
	lua_register(L, "light_spot_angle", ffi_light_spot_angle);
	lua_register(L, "light_spot_blend", ffi_light_spot_blend);
	lua_register(L, "light_use_sphere", ffi_light_use_sphere);
	lua_register(L, "light_use_square", ffi_light_use_square);
	lua_register(L, "light_use_shadow", ffi_light_use_shadow);
}
