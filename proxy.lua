local table_unpack = table.unpack

-- Armatures

local mt_amt = {}

mt_amt.__index = mt_amt

function mt_amt:attach(parent, tagname)
	print("amt_attach", self, parent)
	amt_attach(self.user, parent.user, tagname)
end

function mt_amt:detach()
	amt_detach(self.user)
end

function mt_amt:set_location(x, y, z)
	amt_set_location(self.user, x, y, z)
end

function mt_amt:set_rotation(x, y, z, w)
	amt_set_rotation(self.user, x, y, z, w)
end

function mt_amt:set_scale(x, y, z)
	amt_set_scale(self.user, x, y, z)
end

function mt_amt:location() return amt_location(self.user) end
function mt_amt:rotation() return amt_rotation(self.user) end
function mt_amt:scale() return amt_scale(self.user) end

function mt_amt:play_animation(animname, time)
	amt_play_animation(self.user, animname, time)
end

function mt_amt:stop_animation(animname, time)
	amt_stop_animation(self.user)
end

function armature(data)
	amt = amt_new(data.skel)
	if data.location then amt_set_location(amt, table_unpack(data.location)) end
	if data.rotation then amt_set_rotation(amt, table_unpack(data.rotation)) end
	if data.scale then amt_set_scale(amt, table_unpack(data.scale)) end
	if data.color then amt_set_color(amt, table_unpack(data.color)) end
	return setmetatable({user=amt}, mt_amt)
end

-- Objects

local mt_obj = {}

mt_obj.__index = mt_obj

function mt_obj:attach(parent, tagname)
	obj_attach(self.user, parent.user, tagname)
end

function mt_obj:detach()
	obj_detach(self.user)
end

function mt_obj:set_location(x, y, z)
	obj_set_location(self.user, x, y, z)
end

function mt_obj:set_rotation(x, y, z, w)
	obj_set_rotation(self.user, x, y, z, w)
end

function mt_obj:set_scale(x, y, z)
	obj_set_scale(self.user, x, y, z)
end

function mt_obj:location() return obj_location(self.user) end
function mt_obj:rotation() return obj_rotation(self.user) end
function mt_obj:scale() return obj_scale(self.user) end

function object(data)
	obj = obj_new(data.mesh)
	if data.location then obj_set_location(obj, table_unpack(data.location)) end
	if data.rotation then obj_set_rotation(obj, table_unpack(data.rotation)) end
	if data.scale then obj_set_scale(obj, table_unpack(data.scale)) end
	if data.color then obj_set_color(obj, table_unpack(data.color)) end
	return setmetatable({user=obj}, mt_obj)
end

instance = object

-- Lights

local mt_light = {}

mt_light.__index = mt_light

function mt_light:attach(parent, tagname)
	light_attach(self.user, parent.user, tagname)
end

function mt_light:detach()
	light_detach(self.user)
end

function mt_light:set_location(x, y, z)
	light_set_location(self.user, x, y, z)
end

function mt_light:set_rotation(x, y, z, w)
	light_set_rotation(self.user, x, y, z, w)
end

function mt_light:location() return light_location(self.user) end
function mt_light:rotation() return light_rotation(self.user) end

function light(data)
	light = light_new()
	if data.location then light_set_location(light, table_unpack(data.location)) end
	if data.rotation then light_set_rotation(light, table_unpack(data.rotation)) end
	if data.type then light_set_type(light, data.type) end
	if data.energy then light_set_energy(light, data.energy) end
	if data.color then light_set_color(light, table_unpack(data.color)) end
	if data.distance then light_set_distance(light, data.distance) end
	if data.use_sphere then light_set_use_sphere(light, data.use_sphere) end
	if data.spot_size then light_set_spot_size(light, data.spot_size) end
	if data.spot_blend then light_set_spot_blend(light, data.spot_blend) end
	if data.use_square then light_set_use_square(light, data.use_square) end
	if data.use_shadow then light_set_use_shadow(light, data.use_shadow) end
	return setmetatable({user=light}, mt_light)
end

-- Utilities

function attach(parent, child, tag)
	child:attach(parent, tag)
end
