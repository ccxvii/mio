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
		if (i > 1) console_putc('\t');
		console_print(s);
		lua_pop(L, 1); /* pop result */
	}
	console_putc('\n');
	return 0;
}

static int ffi_traceback(lua_State *L)
{
	const char *msg = lua_tostring(L, 1);
	if (msg) {
		fprintf(stderr, "%s\n", msg);
		luaL_traceback(L, L, msg, 1);
	}
	else if (!lua_isnoneornil(L, 1)) { /* is there an error object? */
		if (!luaL_callmeta(L, 1, "__tostring")) /* try its 'tostring' metamethod */
			lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static int docall(lua_State *L, int narg, int nres)
{
	int status;
	int base = lua_gettop(L) - narg;
	lua_pushcfunction(L, ffi_traceback);
	lua_insert(L, base);
	status = lua_pcall(L, narg, nres, base);
	lua_remove(L, base);
	return status;
}

void run_string(const char *cmd)
{
	if (!L) {
		console_printnl("[no command interpreter]");
		return;
	}

	int status = luaL_loadstring(L, cmd);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}

	status = docall(L, 0, LUA_MULTRET);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
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
		console_printnl("[no command interpreter]");
		return;
	}

	int status = luaL_loadfile(L, filename);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}

	status = docall(L, 0, 0);
	if (status) {
		const char *msg = lua_tostring(L, -1);
		console_printnl(msg);
		lua_pop(L, 1);
		return;
	}
}

static void *checktag(lua_State *L, int n, int tag)
{
	luaL_checktype(L, n, LUA_TLIGHTUSERDATA);
	enum tag *p = lua_touserdata(L, n);
	if (!p || *p != tag)
		luaL_argerror(L, n, "wrong userdata type");
	return p;
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

static int ffi_load_font(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct font *font = load_font(name);
	if (!font)
		return luaL_error(L, "cannot load font: %s", name);
	lua_pushlightuserdata(L, font);
	return 1;
}

/* Model */

static int ffi_load_skel(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct skel *skel = load_skel(name);
	if (!skel)
		return luaL_error(L, "cannot load skel: %s", name);
	lua_pushlightuserdata(L, skel);
	return 1;
}

static int ffi_load_mesh(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct mesh *mesh = load_mesh(name);
	if (!mesh)
		return luaL_error(L, "cannot load mesh: %s", name);
	lua_pushlightuserdata(L, mesh);
	return 1;
}

static int ffi_load_anim(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct anim *anim = load_anim(name);
	if (!anim)
		return luaL_error(L, "cannot load anim: %s", name);
	lua_pushlightuserdata(L, anim);
	return 1;
}

/* Scene */

static int ffi_scn_new(lua_State *L)
{
	struct scene *scn = new_scene();
	lua_pushlightuserdata(L, scn);
	return 1;
}

/* Entity */

static int ffi_ent_new(lua_State *L)
{
	// struct scene *scn = checktag(L, 1, TAG_SCENE);
	// struct entity *ent = ent_new(scn);
	struct entity *ent = new_entity(scene);
	lua_pushlightuserdata(L, ent);
	return 1;
}

/* Transform component */

static int ffi_tra_new(lua_State *L)
{
	lua_pushlightuserdata(L, new_transform());
	return 1;
}

static int ffi_ent_set_transform(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	ent->transform = checktag(L, 2, TAG_TRANSFORM);
	return 0;
}

static int ffi_ent_transform(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	lua_pushlightuserdata(L, ent->transform);
	return 1;
}

static int ffi_tra_set_parent(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	struct entity *parent = checktag(L, 2, TAG_ENTITY);
	const char *bone_name = luaL_checkstring(L, 3);
	if (transform_set_parent(tra, parent, bone_name))
		return luaL_error(L, "cannot find bone: %s", bone_name);
	return 0;
}

static int ffi_tra_clear_parent(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	transform_clear_parent(tra);
	return 0;
}

static int ffi_tra_set_position(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	tra->pose.position[0] = luaL_checknumber(L, 2);
	tra->pose.position[1] = luaL_checknumber(L, 3);
	tra->pose.position[2] = luaL_checknumber(L, 4);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_set_rotation(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	tra->pose.rotation[0] = luaL_checknumber(L, 2);
	tra->pose.rotation[1] = luaL_checknumber(L, 3);
	tra->pose.rotation[2] = luaL_checknumber(L, 4);
	tra->pose.rotation[3] = luaL_checknumber(L, 5);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_set_scale(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	tra->pose.scale[0] = luaL_checknumber(L, 2);
	tra->pose.scale[1] = luaL_checknumber(L, 3);
	tra->pose.scale[2] = luaL_checknumber(L, 4);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_position(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	lua_pushnumber(L, tra->pose.position[0]);
	lua_pushnumber(L, tra->pose.position[1]);
	lua_pushnumber(L, tra->pose.position[2]);
	return 3;
}

static int ffi_tra_rotation(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	lua_pushnumber(L, tra->pose.rotation[0]);
	lua_pushnumber(L, tra->pose.rotation[1]);
	lua_pushnumber(L, tra->pose.rotation[2]);
	lua_pushnumber(L, tra->pose.rotation[3]);
	return 4;
}

static int ffi_tra_scale(lua_State *L)
{
	struct transform *tra = checktag(L, 1, TAG_TRANSFORM);
	lua_pushnumber(L, tra->pose.scale[0]);
	lua_pushnumber(L, tra->pose.scale[1]);
	lua_pushnumber(L, tra->pose.scale[2]);
	return 3;
}

/* Skel instance component */

static int ffi_iskel_new(lua_State *L)
{
	struct skel *skel = checktag(L, 1, TAG_SKEL);
	lua_pushlightuserdata(L, new_iskel(skel));
	return 1;
}

static int ffi_ent_set_iskel(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	ent->iskel = checktag(L, 2, TAG_ISKEL);
	return 0;
}

static int ffi_ent_iskel(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	lua_pushlightuserdata(L, ent->iskel);
	return 1;
}


/* Mesh instance component */

static int ffi_imesh_new(lua_State *L)
{
	struct mesh *mesh = checktag(L, 1, TAG_MESH);
	struct imesh *imesh = new_imesh(mesh);
	lua_pushlightuserdata(L, imesh);
	return 1;
}

static int ffi_ent_add_imesh(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	struct imesh *imesh = checktag(L, 2, TAG_IMESH);
	LIST_INSERT_HEAD(&ent->imesh_list, imesh, imesh_next);
	return 0;
}

static int ffi_imesh_set_color(lua_State *L)
{
	struct imesh *imesh = checktag(L, 1, TAG_IMESH);
	imesh->color[0] = luaL_checknumber(L, 2);
	imesh->color[1] = luaL_checknumber(L, 3);
	imesh->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_imesh_color(lua_State *L)
{
	struct imesh *imesh = checktag(L, 1, TAG_IMESH);
	lua_pushnumber(L, imesh->color[0]);
	lua_pushnumber(L, imesh->color[1]);
	lua_pushnumber(L, imesh->color[2]);
	return 3;
}

/* Lamp component */

static const char *lamp_type_enum[] = { "POINT", "SPOT", "SUN", 0 };

static int ffi_lamp_new(lua_State *L)
{
	struct lamp *lamp = new_lamp();
	if (!lamp)
		return luaL_error(L, "cannot create lamp");
	lua_pushlightuserdata(L, lamp);
	return 1;
}

static int ffi_ent_set_lamp(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	ent->lamp = checktag(L, 2, TAG_LAMP);
	return 0;
}

static int ffi_ent_lamp(lua_State *L)
{
	struct entity *ent = checktag(L, 1, TAG_ENTITY);
	lua_pushlightuserdata(L, ent->lamp);
	return 1;
}

static int ffi_lamp_set_color(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->color[0] = luaL_checknumber(L, 2);
	lamp->color[1] = luaL_checknumber(L, 3);
	lamp->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_lamp_set_type(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->type = luaL_checkoption(L, 2, NULL, lamp_type_enum);
	return 0;
}

static int ffi_lamp_set_energy(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->energy = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_distance(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->distance = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_angle(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->spot_angle = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_blend(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->spot_blend = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_use_sphere(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->use_sphere = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_set_use_shadow(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lamp->use_shadow = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_color(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->color[0]);
	lua_pushnumber(L, lamp->color[1]);
	lua_pushnumber(L, lamp->color[2]);
	return 3;
}

static int ffi_lamp_type(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushstring(L, lamp_type_enum[lamp->type]);
	return 1;
}

static int ffi_lamp_energy(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->energy);
	return 1;
}

static int ffi_lamp_distance(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->distance);
	return 1;
}

static int ffi_lamp_spot_angle(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->spot_angle);
	return 1;
}

static int ffi_lamp_spot_blend(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushnumber(L, lamp->spot_blend);
	return 1;
}

static int ffi_lamp_use_sphere(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushboolean(L, lamp->use_sphere);
	return 1;
}

static int ffi_lamp_use_shadow(lua_State *L)
{
	struct lamp *lamp = checktag(L, 1, TAG_LAMP);
	lua_pushboolean(L, lamp->use_shadow);
	return 1;
}

void init_lua(void)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "print", ffi_print);

	lua_register(L, "register_archive", ffi_register_archive);
	lua_register(L, "register_directory", ffi_register_directory);

	/* draw */
	lua_register(L, "load_font", ffi_load_font);

	/* model */
	lua_register(L, "load_skel", ffi_load_skel);
	lua_register(L, "load_mesh", ffi_load_mesh);
	lua_register(L, "load_anim", ffi_load_anim);

	/* components */

	lua_register(L, "tra_new", ffi_tra_new);
	lua_register(L, "tra_set_parent", ffi_tra_set_parent);
	lua_register(L, "tra_clear_parent", ffi_tra_clear_parent);
	lua_register(L, "tra_set_position", ffi_tra_set_position);
	lua_register(L, "tra_set_rotation", ffi_tra_set_rotation);
	lua_register(L, "tra_set_scale", ffi_tra_set_scale);
	lua_register(L, "tra_position", ffi_tra_position);
	lua_register(L, "tra_rotation", ffi_tra_rotation);
	lua_register(L, "tra_scale", ffi_tra_scale);

	lua_register(L, "iskel_new", ffi_iskel_new);
	// animation play back

	lua_register(L, "imesh_new", ffi_imesh_new);
	lua_register(L, "imesh_set_color", ffi_imesh_set_color);
	lua_register(L, "imesh_color", ffi_imesh_color);

	lua_register(L, "lamp_new", ffi_lamp_new);
	lua_register(L, "lamp_set_type", ffi_lamp_set_type);
	lua_register(L, "lamp_set_color", ffi_lamp_set_color);
	lua_register(L, "lamp_set_energy", ffi_lamp_set_energy);
	lua_register(L, "lamp_set_distance", ffi_lamp_set_distance);
	lua_register(L, "lamp_set_spot_angle", ffi_lamp_set_spot_angle);
	lua_register(L, "lamp_set_spot_blend", ffi_lamp_set_spot_blend);
	lua_register(L, "lamp_set_use_sphere", ffi_lamp_set_use_sphere);
	lua_register(L, "lamp_set_use_shadow", ffi_lamp_set_use_shadow);
	lua_register(L, "lamp_type", ffi_lamp_type);
	lua_register(L, "lamp_color", ffi_lamp_color);
	lua_register(L, "lamp_energy", ffi_lamp_energy);
	lua_register(L, "lamp_distance", ffi_lamp_distance);
	lua_register(L, "lamp_spot_angle", ffi_lamp_spot_angle);
	lua_register(L, "lamp_spot_blend", ffi_lamp_spot_blend);
	lua_register(L, "lamp_use_sphere", ffi_lamp_use_sphere);
	lua_register(L, "lamp_use_shadow", ffi_lamp_use_shadow);

	/* entity */
	lua_register(L, "scn_new", ffi_scn_new);
	lua_register(L, "ent_new", ffi_ent_new);
	lua_register(L, "ent_set_transform", ffi_ent_set_transform);
	lua_register(L, "ent_transform", ffi_ent_transform);
	lua_register(L, "ent_set_iskel", ffi_ent_set_iskel);
	lua_register(L, "ent_iskel", ffi_ent_iskel);
	lua_register(L, "ent_add_imesh", ffi_ent_add_imesh);
	lua_register(L, "ent_set_lamp", ffi_ent_set_lamp);
	lua_register(L, "ent_lamp", ffi_ent_lamp);
	// imesh list access
}
