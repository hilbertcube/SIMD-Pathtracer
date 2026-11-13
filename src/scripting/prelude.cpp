// The Lua code run before every scene script
namespace scripting::bindings
{

/*
 * Vectors, and placed(). It is called with the vector metatable and the layout file's path, and
 * returns a function that lists the layout's names nothing asked for.
 */
// Declared in binding.h; defined here without it, so this file needs no sol2
extern const char* const PRELUDE;
const char* const PRELUDE = R"lua(
local vec_metatable, layout_file = ...

-- Vectors: {x, y, z} tables with arithmetic. Any {x, y, z} table is a vector to the renderer.
local methods = {}
local function new(x, y, z) return setmetatable({x, y, z}, vec_metatable) end
vec_metatable.__index = function(v, key)
    if key == "x" then return v[1] elseif key == "y" then return v[2] elseif key == "z" then return v[3] end
    return methods[key]
end
vec_metatable.__add = function(a, b) return new(a[1] + b[1], a[2] + b[2], a[3] + b[3]) end
vec_metatable.__sub = function(a, b) return new(a[1] - b[1], a[2] - b[2], a[3] - b[3]) end
vec_metatable.__unm = function(a) return new(-a[1], -a[2], -a[3]) end
vec_metatable.__mul = function(a, b)
    if type(a) == "number" then return new(a * b[1], a * b[2], a * b[3]) end
    if type(b) == "number" then return new(a[1] * b, a[2] * b, a[3] * b) end
    return new(a[1] * b[1], a[2] * b[2], a[3] * b[3])
end
vec_metatable.__div = function(a, b) return new(a[1] / b, a[2] / b, a[3] / b) end
vec_metatable.__eq = function(a, b) return a[1] == b[1] and a[2] == b[2] and a[3] == b[3] end
vec_metatable.__tostring = function(v) return string.format("vec(%g, %g, %g)", v[1], v[2], v[3]) end

-- vec(x, y, z), or vec(a) for (a, a, a)
function vec(x, y, z)
    if y == nil and z == nil then return new(x, x, x) end
    return new(x, y, z)
end
function methods.length(v) return math.sqrt(v[1] * v[1] + v[2] * v[2] + v[3] * v[3]) end
function dot(a, b) return a[1] * b[1] + a[2] * b[2] + a[3] * b[3] end
function cross(a, b)
    return new(a[2] * b[3] - a[3] * b[2], a[3] * b[1] - a[1] * b[3], a[1] * b[2] - a[2] * b[1])
end
function unit_vector(v) return v / methods.length(v) end

-- placed(name, values): `values`, with whatever the layout file (written by an editor) sets for
-- `name` in their place
local layout = {}
if layout_file then
    local file = io.open(layout_file)
    if file then
        file:close()
        layout = dofile(layout_file)
        if type(layout) ~= "table" then
            error(layout_file .. " should return a table of placements, by name", 0)
        end
    end
end
local asked = {}
function placed(name, values)
    if type(name) ~= "string" or type(values) ~= "table" then
        error("placed takes a name and a table of values, placed(\"name\", {...})", 2)
    end
    asked[name] = true
    local changes = layout[name]
    if changes == nil then return values end
    local result = {}
    for key, value in pairs(values) do result[key] = value end
    for key, value in pairs(changes) do result[key] = value end
    return result
end

return function()
    local unused = {}
    for name in pairs(layout) do
        if not asked[name] then unused[#unused + 1] = name end
    end
    table.sort(unused)
    return unused
end
)lua";

} // namespace scripting::bindings
