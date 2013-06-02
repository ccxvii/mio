-- Simple tostring replacement that will also print table contents.

local tostring_old = tostring

local keywords = {
	["and"] = true,
	["break"] = true,
	["do"] = true,
	["else"] = true,
	["elseif"] = true,
	["end"] = true,
	["false"] = true,
	["for"] = true,
	["function"] = true,
	["if"] = true,
	["in"] = true,
	["local"] = true,
	["nil"] = true,
	["not"] = true,
	["or"] = true,
	["repeat"] = true,
	["return"] = true,
	["then"] = true,
	["true"] = true,
	["until"] = true,
	["while"] = true
}

local function tostring_imp(x, cat, seen)
	if type(x) == 'table' and not seen[x] then
		seen[x] = true
		cat("{")
		local last_i = 0
		for i, v in ipairs(x) do
			if i > 1 then cat(", ") end
			tostring_imp(v, cat, seen)
			last_i = i
		end
		local needs_comma = last_i > 0
		for k, v in pairs(x) do
			if type(k) ~= 'number' or k < 1 or k > last_i or k % 1 ~= 0 then
				if needs_comma then cat(", ") else needs_comma = true end
				if type(k) == 'string' and not keywords[k] and string.match(k, "^[%a_][%a%d_]*$") then
					cat(k) cat("=")
				else
					cat("[") tostring_imp(k, cat, seen) cat("]=")
				end
				tostring_imp(v, cat, seen)
			end
		end
		cat("}")
		seen[x] = nil
	elseif type(x) == 'string' then
		cat(string.format("%q", x))
	elseif type(x) == 'number' or type(x) == 'boolean' or x == nil then
		cat(tostring_old(x))
	else
		cat("<") cat(tostring_old(x)) cat(">")
	end
end

function tostring(x)
	local buf = {}
	tostring_imp(x, function(v) buf[#buf+1] = v end, {})
	return table.concat(buf)
end

function printf(...) print(string.format(...)) end
