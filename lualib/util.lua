local util = {}

function util.create_field(t, k, v)
    if t[k] == nil then
        t[k] = v
    end
    return t[k]
end

function util.table_empty( tb )
    if tb == nil then
        return true
    end
    if type(tb) ~= "table" then
        print("table_empty error type", type(tb))
        return true
    end
    return _G.next( tb ) == nil
end

function util.deep_copy(object)
	local lookup_table = {}
	local function _copy(object)
		if type(object) ~= "table" then
			return object
		elseif lookup_table[object] then
			return lookup_table[object]
		end

		local new_table = {}
		lookup_table[object] = new_table
		for index, value in pairs(object) do
			new_table[_copy(index)] = _copy(value)
		end

		return setmetatable(new_table, getmetatable(object))
	end
    return _copy(object)
end

function util.shallow_copy(obj)
    local newObj = {}
    for k, v in pairs(obj) do
        newObj[k] = v
    end
    return newObj
end

function util.member_copy(table, obj)
    for k,v in pairs(obj) do
        table[k] = v
    end
end

function util.insert_members(t, members)
    for k,v in pairs(members) do
        table.insert(t, v)
    end
end

local function get_type_first_print(t)
	local str = type(t)
	return string.upper(string.sub(str, 1, 1)) .. ":"
end

function util.dump_table(t, indent_input, print)
	local indent = indent_input
	if indent_input == nil then
		indent = 1
	end

	if print == nil then
		print = _G["print"]
	end

	local formatting = string.rep("    ", indent)
	if t == nil then
		print(formatting .. "nil")
	end

	if type(t) ~= "table" then
		print(formatting..get_type_first_print(t)..tostring(t))
		return
	end

	local output_count = 0
	for k,v in pairs(t) do
		local str_k = get_type_first_print(k)
		if type(v) == "table" then
			print(formatting .. str_k .. k .. " -> ")
			util.dump_table(v, indent+1, print)
		else
			print(formatting .. str_k .. k .. " -> " .. get_type_first_print(v) .. tostring(v))
		end
		output_count = output_count + 1
	end

	if output_count == 0 then
		print(formatting .. "{}")
	end
end

function util.get_element_count(table)
    local count = 0
    for k,v in pairs(table) do
        count = count + 1
    end
    return count
end

function util.check_in_array(table, value)
    if type(table) ~= "table" then
        return false
    end
    for k,v in pairs(table) do
        if v == value then
            return true
        end
    end
    return false
end

function util.find_index(table, value)
    if type(table) ~= "table" then
        return 0
    end
    for k, v in pairs(table) do
        if v == value then
            return k
        end
    end
    return 0
end

function util.check_double_in(table)
    if type(table) ~= "table" then
        return false
    end
    local tmp = {}
    for k,v in pairs(table) do
        if tmp[v] then
            return true
        else
            tmp[v] = 1
        end
    end
    return false
end

function util.swap(t, a, b)
    local temp = t[a]
    t[a] = t[b]
    t[b] = temp
end

function util.clamp(value, min, max)
    if value < min then
        return min
    end
    if value > max then
        return max    
    end
    return value
end

return util