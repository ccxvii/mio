local table_unpack = table.unpack

-- Transform

local mt_tra = {}

mt_tra.__index = mt_tra

function new_transform(data)
	local tra = tra_new()
	if data then
		if data.position then tra_set_position(tra, table_unpack(data.position)) end
		if data.rotation then tra_set_rotation(tra, table_unpack(data.rotation)) end
		if data.scale then tra_set_scale(tra, table_unpack(data.scale)) end
	end
	return setmetatable({user=tra}, mt_tra)
end

function mt_tra:set_parent(parent, tagname)
	tra_set_parent(self.user, parent.user, tagname)
end

function mt_tra:clear_parent()
	tra_clear_parent(self.user)
end

function mt_tra:attach(child, tagname)
	if type(child) == 'string' then
		child = object(child)
	end
	child:set_parent(self, tagname)
end

function mt_tra:set_position(x, y, z)
	tra_set_position(self.user, x, y, z)
end

function mt_tra:set_rotation(x, y, z, w)
	tra_set_rotation(self.user, x, y, z, w)
end

function mt_tra:set_scale(x, y, z)
	if not y or not z then y = x; z = x end
	tra_set_scale(self.user, x, y, z)
end

function mt_tra:position() return tra_position(self.user) end
function mt_tra:rotation() return tra_rotation(self.user) end
function mt_tra:scale() return tra_scale(self.user) end

-- ISkel

local mt_iskel = {}

mt_iskel.__index = mt_iskel

function new_iskel(skel)
	local iskel = iskel_new(load_skel(skel))
	return setmetatable({user=iskel}, mt_iskel)
end

function mt_iskel:play_animation(animname, transition)
	iskel_play_anim(self.user, load_anim(animname), transition)
end

function mt_iskel:stop_animation()
	iskel_stop_anim(self.user)
end

-- IMesh

local mt_imesh = {}

mt_imesh.__index = mt_imesh

function new_imesh(mesh)
	local imesh = imesh_new(load_mesh(mesh))
	return setmetatable({user=imesh}, mt_imesh)
end

-- Lamp

local mt_lamp = {}

mt_lamp.__index = mt_lamp

function new_lamp(data)
	local lamp = lamp_new()
	if data then
		if data.type then lamp_set_type(lamp, data.type) end
		if data.energy then lamp_set_energy(lamp, data.energy) end
		if data.color then lamp_set_color(lamp, table_unpack(data.color)) end
		if data.distance then lamp_set_distance(lamp, data.distance) end
		if data.spot_angle then lamp_set_spot_angle(lamp, data.spot_angle) end
		if data.spot_blend then lamp_set_spot_blend(lamp, data.spot_blend) end
		if type(data.use_sphere) == 'boolean' then lamp_set_use_sphere(lamp, data.use_sphere) end
		if type(data.use_shadow) == 'boolean' then lamp_set_use_shadow(lamp, data.use_shadow) end
	end
	return setmetatable({user=lamp}, mt_lamp)
end

function mt_lamp:set_distance(v) lamp_set_distance(self.user, v) end
function mt_lamp:set_energy(v) lamp_set_energy(self.user, v) end
function mt_lamp:set_color(r,g,b) lamp_set_color(self.user, r, g, b) end
function mt_lamp:set_spot_angle(v) lamp_set_spot_angle(self.user, v) end
function mt_lamp:set_spot_blend(v) lamp_set_spot_blend(self.user, v) end
function mt_lamp:set_use_sphere(v) lamp_set_use_sphere(self.user, v) end
function mt_lamp:set_use_shadow(v) lamp_set_use_shadow(self.user, v) end

function mt_lamp:distance() return lamp_distance(self.user) end
function mt_lamp:energy() return lamp_energy(self.user) end
function mt_lamp:color() return lamp_color(self.user) end
function mt_lamp:spot_angle() return lamp_spot_angle(self.user) end
function mt_lamp:spot_blend() return lamp_spot_blend(self.user) end
function mt_lamp:use_sphere() return lamp_use_sphere(self.user) end
function mt_lamp:use_shadow() return lamp_use_shadow(self.user) end

-- Entity

local mt_ent = {}

mt_ent.__index = mt_ent

function new_entity()
	local ent = ent_new()
	return setmetatable({user=ent}, mt_ent)
end

function mt_ent:set_transform(tra)
	ent_set_transform(self.user, tra.user)
end

function mt_ent:set_iskel(iskel)
	ent_set_iskel(self.user, iskel.user)
end

function mt_ent:add_imesh(imesh)
	ent_add_imesh(self.user, imesh.user)
end

function mt_ent:set_lamp(lamp)
	ent_set_lamp(self.user, lamp.user)
end

-- Utilities

function model(filename)
	local ent = new_entity()
	ent:set_transform(new_transform())
	ent:set_iskel(new_iskel(filename))
	ent:add_imesh(new_imesh(filename))
	return ent
end

function armature(data)
	if type(data) == 'string' then return armature {skel=data} end
	local ent = new_entity()
	ent:set_transform(new_transform(data))
	ent:set_iskel(new_iskel(data.skel))
	return ent
end

function object(data)
	if type(data) == 'string' then return object {mesh=data} end
	local ent = new_entity()
	ent:set_transform(new_transform(data))
	ent:add_imesh(new_imesh(data.mesh))
	return ent
end

function lamp(data)
	if type(data) == 'string' then return lamp {type=data} end
	local ent = new_entity()
	ent:set_transform(new_transform(data))
	ent:set_lamp(new_lamp(data))
	return ent
end
