local table_unpack = table.unpack

meshlist = {}
lamplist = {}

function draw_geometry()
	for k, ent in pairs(meshlist) do
		draw_mesh(ent.transform, ent.mesh)
	end
end

function draw_light()
	for k, ent in pairs(lamplist) do
		draw_lamp(ent.transform, ent.lamp)
	end
end

register_directory "data/trees"
register_directory "data/trees/textures"

meshlist[1] = {
	transform = tra_new(),
	mesh = load_mesh "object_Plane",
}

meshlist[2] = {
	transform = tra_new(),
	mesh = load_mesh "pr_s2_mycotree_a",
}

lamplist[1] = {
	transform = tra_new(),
	lamp = lamp_new(),
}

lamplist[2] = {
	transform = tra_new(),
	lamp = lamp_new(),
}

lamplist[1].transform:set_position(-3.82228, -4.17761, 7.82267)
lamplist[1].lamp:set_type('SPOT')
lamplist[1].lamp:set_spot_angle(45)

lamplist[2].transform:set_position(4.3244, 3.84716, 3.44613)
lamplist[2].lamp:set_type('POINT')
lamplist[2].lamp:set_distance(30)


meshlist[1].transform:set_scale(7.5, 7.5, 7.5)

meshlist[2].transform:set_position(4.7, 0, 0)
meshlist[2].transform:set_scale(0.3, 0.3, 0.3)

--[[
lamp {
	type = 'SPOT',
	position = {
	rotation = {0, 0, 0, 1},

	color = {1, 1, 1},
	energy = 1,
	distance = 25,
	use_sphere = false,
	spot_angle = 45,
	spot_blend = 0.15,
	use_shadow = true,
}
object {
	mesh = "object_Plane",
	position = {0, 0, 0},
	rotation = {0, 0, 0, 1},
	scale = {7.52856827, 7.52856827, 7.52856827},
	color = {1, 1, 1},
}
lamp {
	type = 'POINT',
	position = {4.3244, 3.84716, 3.44613},
	rotation = {0.169076, 0.272171, 0.75588, 0.570948},
	color = {1, 1, 1},
	energy = 1,
	distance = 30,
	use_sphere = false,
	use_shadow = true,
}
object {
	mesh = "fo_s1_arbragrelot",
	position = {4.67888, 0, 0},
	rotation = {0, 0, 0, 1},
	scale = {0.300000012, 0.300000012, 0.300000012},
}
--]]
