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

static int ffi_amt_new(lua_State *L)
{
	struct armature *amt = new_armature(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, amt);
	return 1;
}

static int ffi_obj_new(lua_State *L)
{
	struct object *obj = new_object(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, obj);
	return 1;
}

static int ffi_light_new(lua_State *L)
{
	struct light *light = new_light(scene);
	lua_pushlightuserdata(L, light);
	return 1;
}

static int ffi_amt_set_position(lua_State *L)
{
	struct armature *amt = checktag(L, 1, TAG_ARMATURE);
	amt->transform[12] = lua_tonumber(L, 2);
	amt->transform[13] = lua_tonumber(L, 3);
	amt->transform[14] = lua_tonumber(L, 4);
	return 0;
}

static int ffi_obj_set_position(lua_State *L)
{
	struct object *obj = checktag(L, 1, TAG_OBJECT);
	obj->transform[12] = lua_tonumber(L, 2);
	obj->transform[13] = lua_tonumber(L, 3);
	obj->transform[14] = lua_tonumber(L, 4);
	return 0;
}

void bind_init(void)
{
	if (!L) {
		L = luaL_newstate();
		luaL_openlibs(L);
	}

	lua_register(L, "amt_new", ffi_amt_new);
	lua_register(L, "obj_new", ffi_obj_new);
	lua_register(L, "light_new", ffi_light_new);
	lua_register(L, "amt_set_position", ffi_amt_set_position);
	lua_register(L, "obj_set_position", ffi_obj_set_position);
}
