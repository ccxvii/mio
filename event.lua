uistate = {}

function on_keyboard(key, mod, x, y)
	print("key", key, mod, x, y)
	if key == 'UP' then mixer = mixer + 0.1 end
	if key == 'DOWN' then mixer = mixer - 0.1 end
	if mixer < 0 then mixer = 0 end
	if mixer > 1 then mixer = 1 end
end

function on_mouse(button, state, mod, x, y)
	print("mouse", button, state, mod, x, y)
end

function on_motion(x, y)
	print("motion", x, y)
end
