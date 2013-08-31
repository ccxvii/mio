local table_unpack = table.unpack
local table_insert = table.insert

local meshlist = {}
local lamplist = {}

function update()
	for k, ent in pairs(meshlist) do
		if ent.skel and ent.anim then
			ent.frame = ent.frame + 0.3
			local n = anim_len(ent.anim)
			if ent.frame >= n then
				ent.frame = ent.frame - n
			end
			ent.skel:animate(ent.anim, ent.frame)
		end
	end
end

function draw_geometry()
	for k, ent in pairs(meshlist) do
		if ent.skel then
			if ent.mesh then
				draw_mesh_skel(ent.transform, ent.mesh, ent.skel)
			end
			if ent.meshlist then
				for i, mesh in pairs(ent.meshlist) do
					draw_mesh_skel(ent.transform, mesh, ent.skel)
				end
			end
		else
			if ent.mesh then
				draw_mesh(ent.transform, ent.mesh)
			end
			if ent.meshlist then
				for i, mesh in pairs(ent.meshlist) do
					draw_mesh(ent.transform, mesh)
				end
			end
		end
	end
end

function draw_light()
	for k, ent in pairs(lamplist) do
		if ent.lamp then
			draw_lamp(ent.transform, ent.lamp)
		end
	end
end

function new_transform(t)
	local tra = tra_new()
	if t then
		if t.position then tra:set_position(table_unpack(t.position)) end
		if t.rotation then tra:set_rotation(table_unpack(t.rotation)) end
		if t.scale then tra:set_scale(table_unpack(t.scale)) end
	end
	return tra
end

function new_lamp(t)
	local lamp = lamp_new()
	if t then
		if t.type then lamp:set_type(t.type) end
		if t.energy then lamp:set_energy(t.energy) end
		if t.color then lamp:set_color(table_unpack(t.color)) end
		if t.distance then lamp:set_distance(t.distance) end
		if t.spot_angle then lamp:set_spot_angle(t.spot_angle) end
		if t.spot_blend then lamp:set_spot_blend(t.spot_blend) end
		if type(t.use_sphere) == 'boolean' then lamp:set_use_sphere(t.use_sphere) end
		if type(t.use_shadow) == 'boolean' then lamp:set_use_shadow(t.use_shadow) end
	end
	return lamp
end

function new_mesh(filename)
	return load_mesh(filename)
end

function new_meshlist(list)
	for k, filename in pairs(list) do
		list[k] = load_mesh(filename)
	end
	return list
end

function new_skel(filename)
	return skel_new(load_skel(filename))
end

function lamp(t)
	if type(t) == 'string' then return lamp {type=t} end
	local ent = {}
	ent.transform = new_transform(t)
	ent.lamp = new_lamp(t)
	table_insert(lamplist, ent)
	return ent
end

function model(filename)
	local ent = {}
	ent.transform = new_transform()
	ent.skel = new_skel(filename)
	ent.mesh = new_mesh(filename)
	table_insert(meshlist, ent)
	return ent
end

function object(t)
	if type(t) == 'string' then return object {mesh=t} end
	local ent = {}
	ent.transform = new_transform(t)
	if t.skel then ent.skel = new_skel(t.skel) end
	if t.mesh then ent.mesh = new_mesh(t.mesh) end
	if t.meshlist then ent.meshlist = new_meshlist(t.meshlist) end
	if t.anim then ent.anim = load_anim(t.anim) end
	if t.frame then ent.frame = t.frame end
	table_insert(meshlist, ent)
	return ent
end

