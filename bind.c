#include "mio.h"

struct scene *scene = NULL;

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

int docall(lua_State *L, int narg, int nres)
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

void run_function(const char *fun)
{
	if (!L) {
		console_printnl("[no command interpreter]");
		return;
	}

	lua_getglobal(L, fun);
	int status = docall(L, 0, 0);
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

/* Mesh */

static int ffi_new_mesh(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct mesh *mesh = load_mesh(name);
	if (!mesh)
		return luaL_error(L, "cannot load mesh: %s", name);
	lua_pushlightuserdata(L, mesh);
	return 1;
}

/* Anim */

static int ffi_new_anim(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct anim *anim = load_anim(name);
	if (!anim)
		return luaL_error(L, "cannot load anim: %s", name);
	lua_pushlightuserdata(L, anim);
	return 1;
}

static int ffi_anim_len(lua_State *L)
{
	struct anim *anim = checktag(L, 1, TAG_ANIM);
	lua_pushnumber(L, anim->frames - 1);
	return 1;
}

/* Transform component */

static int ffi_new_transform(lua_State *L)
{
	struct transform *tra = lua_newuserdata(L, sizeof(struct transform));
	luaL_setmetatable(L, "mio.transform");
	init_transform(tra);
	return 1;
}

static int ffi_tra_set_position(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	tra->position[0] = luaL_checknumber(L, 2);
	tra->position[1] = luaL_checknumber(L, 3);
	tra->position[2] = luaL_checknumber(L, 4);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_set_rotation(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	tra->rotation[0] = luaL_checknumber(L, 2);
	tra->rotation[1] = luaL_checknumber(L, 3);
	tra->rotation[2] = luaL_checknumber(L, 4);
	tra->rotation[3] = luaL_checknumber(L, 5);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_set_scale(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	tra->scale[0] = luaL_checknumber(L, 2);
	tra->scale[1] = luaL_checknumber(L, 3);
	tra->scale[2] = luaL_checknumber(L, 4);
	tra->dirty = 1;
	return 0;
}

static int ffi_tra_position(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	lua_pushnumber(L, tra->position[0]);
	lua_pushnumber(L, tra->position[1]);
	lua_pushnumber(L, tra->position[2]);
	return 3;
}

static int ffi_tra_rotation(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	lua_pushnumber(L, tra->rotation[0]);
	lua_pushnumber(L, tra->rotation[1]);
	lua_pushnumber(L, tra->rotation[2]);
	lua_pushnumber(L, tra->rotation[3]);
	return 4;
}

static int ffi_tra_scale(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	lua_pushnumber(L, tra->scale[0]);
	lua_pushnumber(L, tra->scale[1]);
	lua_pushnumber(L, tra->scale[2]);
	return 3;
}

static luaL_Reg ffi_tra_funs[] = {
	{ "set_position", ffi_tra_set_position },
	{ "set_rotation", ffi_tra_set_rotation },
	{ "set_scale", ffi_tra_set_scale },
	{ "position", ffi_tra_position },
	{ "rotation", ffi_tra_rotation },
	{ "scale", ffi_tra_scale },
	{ NULL, NULL }
};

/* Skel component */

static int ffi_new_skel(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct skel *skel = load_skel(name);
	if (!skel)
		return luaL_error(L, "cannot load skel: %s", name);
	struct skelpose *skelpose = lua_newuserdata(L, sizeof(struct skelpose));
	luaL_setmetatable(L, "mio.skel");
	init_skelpose(skelpose, skel);
	return 1;
}

static int ffi_skel_animate(lua_State *L)
{
	struct skelpose *skelpose = luaL_checkudata(L, 1, "mio.skel");
	struct anim *anim = checktag(L, 2, TAG_ANIM);
	float frame = luaL_checknumber(L, 3);
	float blend = luaL_checknumber(L, 4);
	animate_skelpose(skelpose, anim, frame, blend);
	return 0;
}

static luaL_Reg ffi_skel_funs[] = {
	{ "animate", ffi_skel_animate },
	{ NULL, NULL }
};

/* Lamp component */

static const char *lamp_type_enum[] = { "POINT", "SPOT", "SUN", 0 };

static int ffi_new_lamp(lua_State *L)
{
	struct lamp *lamp = lua_newuserdata(L, sizeof(struct lamp));
	luaL_setmetatable(L, "mio.lamp");
	init_lamp(lamp);
	return 1;
}

static int ffi_lamp_set_type(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->type = luaL_checkoption(L, 2, NULL, lamp_type_enum);
	return 0;
}

static int ffi_lamp_set_color(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->color[0] = luaL_checknumber(L, 2);
	lamp->color[1] = luaL_checknumber(L, 3);
	lamp->color[2] = luaL_checknumber(L, 4);
	return 0;
}

static int ffi_lamp_set_energy(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->energy = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_distance(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->distance = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_angle(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->spot_angle = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_spot_blend(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->spot_blend = luaL_checknumber(L, 2);
	return 0;
}

static int ffi_lamp_set_use_sphere(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->use_sphere = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_set_use_shadow(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lamp->use_shadow = lua_toboolean(L, 2);
	return 0;
}

static int ffi_lamp_type(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushstring(L, lamp_type_enum[lamp->type]);
	return 1;
}

static int ffi_lamp_color(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushnumber(L, lamp->color[0]);
	lua_pushnumber(L, lamp->color[1]);
	lua_pushnumber(L, lamp->color[2]);
	return 3;
}

static int ffi_lamp_energy(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushnumber(L, lamp->energy);
	return 1;
}

static int ffi_lamp_distance(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushnumber(L, lamp->distance);
	return 1;
}

static int ffi_lamp_spot_angle(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushnumber(L, lamp->spot_angle);
	return 1;
}

static int ffi_lamp_spot_blend(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushnumber(L, lamp->spot_blend);
	return 1;
}

static int ffi_lamp_use_sphere(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushboolean(L, lamp->use_sphere);
	return 1;
}

static int ffi_lamp_use_shadow(lua_State *L)
{
	struct lamp *lamp = luaL_checkudata(L, 1, "mio.lamp");
	lua_pushboolean(L, lamp->use_shadow);
	return 1;
}

static luaL_Reg ffi_lamp_funs[] = {
	{ "set_type", ffi_lamp_set_type },
	{ "set_color", ffi_lamp_set_color },
	{ "set_energy", ffi_lamp_set_energy },
	{ "set_distance", ffi_lamp_set_distance },
	{ "set_spot_angle", ffi_lamp_set_spot_angle },
	{ "set_spot_blend", ffi_lamp_set_spot_blend },
	{ "set_use_sphere", ffi_lamp_set_use_sphere },
	{ "set_use_shadow", ffi_lamp_set_use_shadow },
	{ "type", ffi_lamp_type },
	{ "color", ffi_lamp_color },
	{ "energy", ffi_lamp_energy },
	{ "distance", ffi_lamp_distance },
	{ "spot_angle", ffi_lamp_spot_angle },
	{ "spot_blend", ffi_lamp_spot_blend },
	{ "use_sphere", ffi_lamp_use_sphere },
	{ "use_shadow", ffi_lamp_use_shadow },
	{ NULL, NULL }
};

/* Render functions */

static int ffi_update_transform(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	update_transform(tra);
	return 0;
}

static int ffi_update_transform_parent(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	struct transform *par = luaL_checkudata(L, 2, "mio.transform");
	update_transform_parent(tra, par);
	return 0;
}

static int ffi_update_transform_parent_skel(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	struct transform *par = luaL_checkudata(L, 2, "mio.transform");
	struct skelpose *skel = luaL_checkudata(L, 3, "mio.skel");
	const char *bone = luaL_checkstring(L, 4);
	update_transform_parent_skel(tra, par, skel, bone);
	return 0;
}

static int ffi_draw_mesh(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	struct mesh *mesh = checktag(L, 2, TAG_MESH);
	render_mesh(tra, mesh);
	return 0;
}

static int ffi_draw_mesh_skel(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	struct mesh *mesh = checktag(L, 2, TAG_MESH);
	struct skelpose *skelpose = luaL_checkudata(L, 3, "mio.skel");
	render_mesh_skel(tra, mesh, skelpose);
	return 0;
}

static int ffi_draw_lamp(lua_State *L)
{
	struct transform *tra = luaL_checkudata(L, 1, "mio.transform");
	struct lamp *lamp = luaL_checkudata(L, 2, "mio.lamp");
	render_lamp(tra, lamp);
	return 0;
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
	lua_register(L, "new_skel", ffi_new_skel);
	lua_register(L, "new_mesh", ffi_new_mesh);
	lua_register(L, "new_anim", ffi_new_anim);

	lua_register(L, "anim_len", ffi_anim_len); // metatable!?

	/* components */

	lua_register(L, "new_transform_imp", ffi_new_transform);
	lua_register(L, "new_lamp_imp", ffi_new_lamp);

	if (luaL_newmetatable(L, "mio.skel")) {
		luaL_setfuncs(L, ffi_skel_funs, 0);
		lua_pushvalue(L, 1);
		lua_setfield(L, 1, "__index");
		lua_pop(L, 1);
	}

	if (luaL_newmetatable(L, "mio.transform")) {
		luaL_setfuncs(L, ffi_tra_funs, 0);
		lua_pushvalue(L, 1);
		lua_setfield(L, 1, "__index");
		lua_pop(L, 1);
	}

	if (luaL_newmetatable(L, "mio.lamp")) {
		luaL_setfuncs(L, ffi_lamp_funs, 0);
		lua_pushvalue(L, 1);
		lua_setfield(L, 1, "__index");
		lua_pop(L, 1);
	}

	/* rendering */
	lua_register(L, "update_transform", ffi_update_transform);
	lua_register(L, "update_transform_parent", ffi_update_transform_parent);
	lua_register(L, "update_transform_parent_skel", ffi_update_transform_parent_skel);
	lua_register(L, "draw_mesh", ffi_draw_mesh);
	lua_register(L, "draw_mesh_skel", ffi_draw_mesh_skel);
	lua_register(L, "draw_lamp", ffi_draw_lamp);
}
