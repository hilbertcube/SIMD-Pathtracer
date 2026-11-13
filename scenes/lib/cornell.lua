--[[
The Cornell box: 555 on a side, open toward the camera (-z), lit from the ceiling. Use it as
    cornell.walls(); add(...); cornell.camera{image_width = 600, samples_per_pixel = 100}
]]

local lights = require "lights"

local cornell = {}

cornell.red   = lambertian{albedo = {0.65, 0.05, 0.05}}
cornell.white = lambertian{albedo = 0.73}
cornell.green = lambertian{albedo = {0.12, 0.45, 0.15}}
cornell.light = diffuse_light{emission = 15}

-- The walls and the ceiling light, with `back` (default white) on the back wall
function cornell.walls(back)
    add(quad{corner = {555, 0, 0}, u = {0, 555, 0}, v = {0, 0, 555}, material = cornell.green})
    add(quad{corner = {0, 0, 0}, u = {0, 555, 0}, v = {0, 0, 555}, material = cornell.red})
    lights.panel({343, 554, 332}, {-130, 0, 0}, {0, 0, -105}, cornell.light)
    add(quad{corner = {0, 0, 0}, u = {555, 0, 0}, v = {0, 0, 555}, material = cornell.white})
    add(quad{corner = {555, 555, 555}, u = {-555, 0, 0}, v = {0, 0, -555}, material = cornell.white})
    add(quad{corner = {0, 0, 555}, u = {555, 0, 0}, v = {0, 555, 0}, material = back or cornell.white})
end

-- The camera looking into the box; `settings` adds to or replaces its settings
function cornell.camera(settings)
    local all = {
        aspect_ratio = 1,
        max_depth = 50,
        background = 0,
        field_of_view = 40,
        look_from = {278, 278, -800},
        look_at = {278, 278, 0},
    }
    for key, value in pairs(settings) do all[key] = value end
    camera(all)
end

return cornell
