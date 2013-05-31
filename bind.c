#include "mio.h"

struct scene *scene = NULL;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *L = NULL;

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
	struct armature *amt = new_armature(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, amt);
	return 1;
}

static int ffi_amt_set_location(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->location[0] = lua_tonumber(L, 2);
	amt->location[1] = lua_tonumber(L, 3);
	amt->location[2] = lua_tonumber(L, 4);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_rotation(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->rotation[0] = lua_tonumber(L, 2);
	amt->rotation[1] = lua_tonumber(L, 3);
	amt->rotation[2] = lua_tonumber(L, 4);
	amt->rotation[3] = lua_tonumber(L, 5);
	amt->dirty = 1;
	return 0;
}

static int ffi_amt_set_scale(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->scale[0] = lua_tonumber(L, 2);
	amt->scale[1] = lua_tonumber(L, 3);
	amt->scale[2] = lua_tonumber(L, 4);
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

/* Object */

static int ffi_obj_new(lua_State *L)
{
	struct object *obj = new_object(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, obj);
	return 1;
}

static int ffi_obj_set_location(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->location[0] = lua_tonumber(L, 2);
	obj->location[1] = lua_tonumber(L, 3);
	obj->location[2] = lua_tonumber(L, 4);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_rotation(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->rotation[0] = lua_tonumber(L, 2);
	obj->rotation[1] = lua_tonumber(L, 3);
	obj->rotation[2] = lua_tonumber(L, 4);
	obj->rotation[3] = lua_tonumber(L, 5);
	obj->dirty = 1;
	return 0;
}

static int ffi_obj_set_scale(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->scale[0] = lua_tonumber(L, 2);
	obj->scale[1] = lua_tonumber(L, 3);
	obj->scale[2] = lua_tonumber(L, 4);
	obj->dirty = 1;
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

/* Light */

static int ffi_light_new(lua_State *L)
{
	struct light *light = new_light(scene);
	lua_pushlightuserdata(L, light);
	return 1;
}

static int ffi_light_set_location(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->location[0] = lua_tonumber(L, 2);
	light->location[1] = lua_tonumber(L, 3);
	light->location[2] = lua_tonumber(L, 4);
	light->dirty = 1;
	return 0;
}

static int ffi_light_set_rotation(lua_State *L)
{
	struct light *light = checktag(L, 1, TAG_LIGHT);
	light->rotation[0] = lua_tonumber(L, 2);
	light->rotation[1] = lua_tonumber(L, 3);
	light->rotation[2] = lua_tonumber(L, 4);
	light->rotation[3] = lua_tonumber(L, 5);
	light->dirty = 1;
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

/* Misc */

static int ffi_register_archive(lua_State *L)
{
	register_archive(lua_tostring(L, 1));
}

static int ffi_register_directory(lua_State *L)
{
	register_directory(lua_tostring(L, 1));
}

void bind_init(void)
{
	if (!L) {
		L = luaL_newstate();
		luaL_openlibs(L);
	}

	lua_register(L, "register_archive", ffi_register_archive);
	lua_register(L, "register_directory", ffi_register_directory);

	lua_register(L, "amt_new", ffi_amt_new);
	lua_register(L, "amt_set_location", ffi_amt_set_location);
	lua_register(L, "amt_set_rotation", ffi_amt_set_rotation);
	lua_register(L, "amt_set_scale", ffi_amt_set_scale);
	lua_register(L, "amt_location", ffi_amt_location);
	lua_register(L, "amt_rotation", ffi_amt_rotation);
	lua_register(L, "amt_scale", ffi_amt_scale);

	lua_register(L, "obj_new", ffi_obj_new);
	lua_register(L, "obj_set_location", ffi_obj_set_location);
	lua_register(L, "obj_set_rotation", ffi_obj_set_rotation);
	lua_register(L, "obj_set_scale", ffi_obj_set_scale);
	lua_register(L, "obj_location", ffi_obj_location);
	lua_register(L, "obj_rotation", ffi_obj_rotation);
	lua_register(L, "obj_scale", ffi_obj_scale);

	lua_register(L, "light_new", ffi_light_new);
	lua_register(L, "light_set_location", ffi_light_set_location);
	lua_register(L, "light_set_rotation", ffi_light_set_rotation);
	lua_register(L, "light_location", ffi_light_location);
	lua_register(L, "light_rotation", ffi_light_rotation);
}
