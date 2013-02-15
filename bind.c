#include "mio.h"

struct scene *scene = NULL;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *L = NULL;

static int ffi_new_armature(lua_State *L)
{
	struct armature *amt = new_armature(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, amt);
	return 1;
}

static int ffi_new_object(lua_State *L)
{
	struct object *obj = new_object(scene, lua_tostring(L, 1));
	lua_pushlightuserdata(L, obj);
	return 1;
}

static int ffi_new_light(lua_State *L)
{
	struct light *light = new_light(scene);
	lua_pushlightuserdata(L, light);
	return 1;
}

static int ffi_amt_set_position(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct armature *amt = lua_touserdata(L, 1);
	amt->transform[12] = lua_tonumber(L, 2);
	amt->transform[13] = lua_tonumber(L, 3);
	amt->transform[14] = lua_tonumber(L, 4);
	return 0;
}

static int ffi_obj_set_position(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct object *obj = lua_touserdata(L, 1);
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

	lua_register(L, "new_armature", ffi_new_armature);
	lua_register(L, "new_object", ffi_new_object);
	lua_register(L, "new_light", ffi_new_light);
	lua_register(L, "amt_set_position", ffi_amt_set_position);
	lua_register(L, "obj_set_position", ffi_obj_set_position);
}
