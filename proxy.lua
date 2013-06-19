local table_unpack = table.unpack

-- Armatures

local mt_amt = {}

mt_amt.__index = mt_amt

function mt_amt:set_parent(parent, tagname)
	amt_set_parent(self.user, parent.user, tagname)
end

function mt_amt:clear_parent()
	amt_clear_parent(self.user)
end

function mt_amt:attach(child, tagname)
	if type(child) == 'string' then
		child = object(child)
	end
	child:set_parent(self, tagname)
end

function mt_amt:set_position(x, y, z)
	amt_set_position(self.user, x, y, z)
end

function mt_amt:set_rotation(x, y, z, w)
	amt_set_rotation(self.user, x, y, z, w)
end

function mt_amt:set_scale(x, y, z)
	if not y or not z then y = x; z = x end
	amt_set_scale(self.user, x, y, z)
end

function mt_amt:position() return amt_position(self.user) end
function mt_amt:rotation() return amt_rotation(self.user) end
function mt_amt:scale() return amt_scale(self.user) end

function mt_amt:play_animation(animname, transition)
	amt_play_anim(self.user, load_anim(animname), transition)
end

function mt_amt:stop_animation()
	amt_stop_anim(self.user)
end

function armature(data)
	if type(data) == 'string' then return armature {skel=data} end
	local amt = amt_new(load_skel(data.skel))
	if data.position then amt_set_position(amt, table_unpack(data.position)) end
	if data.rotation then amt_set_rotation(amt, table_unpack(data.rotation)) end
	if data.scale then amt_set_scale(amt, table_unpack(data.scale)) end
	if data.color then amt_set_color(amt, table_unpack(data.color)) end
	return setmetatable({user=amt}, mt_amt)
end

-- Objects

local mt_obj = {}

mt_obj.__index = mt_obj

function mt_obj:set_parent(parent, tagname)
	obj_set_parent(self.user, parent.user, tagname)
end

function mt_obj:clear_parent()
	obj_clear_parent(self.user)
end

function mt_obj:set_position(x, y, z)
	obj_set_position(self.user, x, y, z)
end

function mt_obj:set_rotation(x, y, z, w)
	obj_set_rotation(self.user, x, y, z, w)
end

function mt_obj:set_scale(x, y, z)
	if not y or not z then y = x; z = x end
	obj_set_scale(self.user, x, y, z)
end

function mt_obj:position() return obj_position(self.user) end
function mt_obj:rotation() return obj_rotation(self.user) end
function mt_obj:scale() return obj_scale(self.user) end

function object(data)
	if type(data) == 'string' then return object {mesh=data} end
	local obj = obj_new(load_mesh(data.mesh))
	if data.position then obj_set_position(obj, table_unpack(data.position)) end
	if data.rotation then obj_set_rotation(obj, table_unpack(data.rotation)) end
	if data.scale then obj_set_scale(obj, table_unpack(data.scale)) end
	if data.color then obj_set_color(obj, table_unpack(data.color)) end
	return setmetatable({user=obj}, mt_obj)
end

instance = object

function empty(data) end

-- Lights

local mt_lamp = {}

mt_lamp.__index = mt_lamp

function mt_lamp:set_parent(parent, tagname)
	lamp_set_parent(self.user, parent.user, tagname)
end

function mt_lamp:clear_parent()
	lamp_clear_parent(self.user)
end

function mt_lamp:set_position(x, y, z)
	lamp_set_position(self.user, x, y, z)
end

function mt_lamp:set_rotation(x, y, z, w)
	lamp_set_rotation(self.user, x, y, z, w)
end

function mt_lamp:position() return lamp_position(self.user) end
function mt_lamp:rotation() return lamp_rotation(self.user) end

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

function lamp(data)
	if type(data) == 'string' then return lamp {type=data} end
	local lamp = lamp_new()
	if data.type then lamp_set_type(lamp, data.type) end
	if data.position then lamp_set_position(lamp, table_unpack(data.position)) end
	if data.rotation then lamp_set_rotation(lamp, table_unpack(data.rotation)) end
	if data.energy then lamp_set_energy(lamp, data.energy) end
	if data.color then lamp_set_color(lamp, table_unpack(data.color)) end
	if data.distance then lamp_set_distance(lamp, data.distance) end
	if data.spot_angle then lamp_set_spot_angle(lamp, data.spot_angle) end
	if data.spot_blend then lamp_set_spot_blend(lamp, data.spot_blend) end
	if type(data.use_sphere) == 'boolean' then lamp_set_use_sphere(lamp, data.use_sphere) end
	if type(data.use_shadow) == 'boolean' then lamp_set_use_shadow(lamp, data.use_shadow) end
	return setmetatable({user=lamp}, mt_lamp)
end

-- Utilities

function model(filename)
	local amt = armature(filename)
	local obj = object(filename)
	obj:set_parent(amt)
	return amt
end
