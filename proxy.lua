local table_unpack = table.unpack
local table_insert = table.insert

local meshlist = {}
local lamplist = {}

entities = {}
actions = {}

function playonce(skel, anim)
	local frame = 0
	local n = anim_len(anim)
	while frame < n do
		skel:animate(anim, frame, 1)
		coroutine.yield()
		frame = frame + 1
	end
end

mixer = 0

function playmix2(skel, a, b)
	local a_i, a_n = 0, anim_len(a)
	local b_i, b_n = 0, anim_len(b)
	local a_step = a_n / b_n
	while true do
		skel:animate(a, a_i, 1)
		skel:animate(b, b_i, mixer)
		coroutine.yield()
		a_i = a_i + a_step
		b_i = b_i + 1
		if a_i >= a_n then a_i = a_i - a_n end
		if b_i >= b_n then b_i = b_i - b_n end
	end
end

function playmix(skel, a, b, duration)
	local a_i, a_n = 0, anim_len(a)
	local b_i, b_n = 0, anim_len(b)
	local a_step = a_n / b_n
	local i = 0
	print("mix", a_n, b_n, a_step)
	for i = 0, duration-1 do
		skel:animate(a, a_i, 1)
		skel:animate(b, b_i, (i / duration))
		coroutine.yield()
		a_i = a_i + a_step
		b_i = b_i + 1
		if a_i >= a_n then a_i = a_i - a_n end
		if b_i >= b_n then b_i = b_i - b_n end
		i = i + 1
	end
	while true do
		skel:animate(b, b_i, 1)
		coroutine.yield()
		b_i = b_i + 1
		if b_i >= b_n then b_i = b_i - b_n end
	end
end

function playseq(skel, list)
	while true do
		for i, anim in ipairs(list) do
			playonce(skel, anim)
		end
	end
end

function playloop(skel, anim, frame)
	local n = anim_len(anim)
	while true do
		skel:animate(anim, frame, 1)
		coroutine.yield()
		frame = frame + 1
		if frame >= n then
			frame = frame - n
		end
	end
end

function run(f)
	table.insert(actions, coroutine.wrap(f))
end

function update()
	for k, action in pairs(actions) do
		action()
	end

	for k, ent in pairs(meshlist) do
		if ent.transform then
			if ent.parent then
				if ent.parentbone then
					update_transform_parent_skel(ent.transform, ent.parent.transform, ent.parent.skel, ent.parentbone)
				else
					update_transform_parent(ent.transform, ent.parent.transform)
				end
			else
				update_transform(ent.transform)
			end
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
	local tra = new_transform_imp()
	if t then
		if t.position then tra:set_position(table_unpack(t.position)) end
		if t.rotation then tra:set_rotation(table_unpack(t.rotation)) end
		if t.scale then tra:set_scale(table_unpack(t.scale)) end
	end
	return tra
end

function new_lamp(t)
	local lamp = new_lamp_imp()
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

function new_meshlist(list)
	for k, filename in pairs(list) do
		list[k] = new_mesh(filename)
	end
	return list
end

function entity(t)
	if t.name then
		entities[t.name] = t
	end

	t.transform = new_transform(t.transform)

	if t.lamp then t.lamp = new_lamp(t.lamp) end
	if t.skel then t.skel = new_skel(t.skel) end
	if t.mesh then t.mesh = new_mesh(t.mesh) end
	if t.meshlist then t.meshlist = new_meshlist(t.meshlist) end

	if t.mesh or t.meshlist then table_insert(meshlist, t) end
	if t.lamp then table_insert(lamplist, t) end

	return t
end
