--[[
Lights the shadow rays aim at. `emission` is a color, or a light material to share; `weight`
(default 1) is how often the shadow rays aim at it, 0 for never.
]]

local lights = {}

local function emitter(emission)
    if type(emission) == "userdata" then return emission end
    return diffuse_light{emission = emission}
end

-- An emitting panel from `corner` along the edges `u` and `v`, facing along u x v
function lights.panel(corner, u, v, emission, weight)
    local panel = add(quad{corner = corner, u = u, v = v, material = emitter(emission)})
    light(panel, weight)
    return panel
end

-- An emitting sphere
function lights.bulb(center, radius, emission, weight)
    local bulb = add(sphere{center = center, radius = radius, material = emitter(emission)})
    light(bulb, weight)
    return bulb
end

--[[
A sun far away in `direction` (a unit vector), as big as the real one unless `half_angle` says
otherwise; `color` is how brightly it lights a surface facing it
]]
function lights.sun(settings)
    local distance    = settings.distance or 10000
    local half_angle  = settings.half_angle or 0.00465 -- the real sun's: 0.53 degrees across
    local solid_angle = 2 * math.pi * (1 - math.cos(half_angle))
    return lights.bulb(distance * settings.direction, distance * math.tan(half_angle),
                       vec(table.unpack(settings.color)) / solid_angle, settings.weight)
end

return lights
