-- A wooden kitchen with a marble island (350k triangles), lit by a window and pendant lamps

local lights = require "lights"

output "kitchen.png"

-- A palette for the other parts (appliances, dishes, tools), picked by a hash of the name
local palette = {
    conductor{reflectance = {0.78, 0.79, 0.81}, roughness = 0.25}, -- stainless steel
    conductor{reflectance = {0.95, 0.70, 0.32}, roughness = 0.30}, -- brass
    coated{albedo = {0.85, 0.83, 0.78}},                           -- cream ceramic
    coated{albedo = {0.08, 0.08, 0.09}},                           -- black plastic
    coated{albedo = {0.55, 0.30, 0.15}},                           -- light wood
    oren_nayar{albedo = {0.70, 0.68, 0.62}, sigma = 0.5},          -- stoneware
    coated{albedo = {0.65, 0.12, 0.10}},                           -- red enamel
    conductor{reflectance = {0.55, 0.56, 0.58}, roughness = 0.40}, -- brushed metal
}

-- Materials for the parts with telling names; the model's own .mtl isn't used
local walls = lambertian{albedo = {0.82, 0.78, 0.70}}
local wood  = coated{albedo = {0.38, 0.22, 0.11}}
local black = coated{albedo = {0.02, 0.02, 0.025}}
local named = {
    ["walls"]               = walls,
    ["Material"]            = walls,
    ["Material.012"]        = oren_nayar{albedo = {0.36, 0.32, 0.28}, sigma = 0.5}, -- the floor
    ["Material.004"]        = dielectrics{refraction_index = 1.5},                  -- the window panes
    ["wooden_panel"]        = wood,
    ["wood"]                = wood,
    ["wooden_seat"]         = wood,
    ["marble"]              = coated{albedo = {0.82, 0.82, 0.84}},
    ["porcelain"]           = coated{albedo = {0.88, 0.88, 0.86}},
    ["black"]               = black,
    ["wall_black.001"]      = black,
    ["leaves_alpha_masked"] = lambertian{albedo = {0.12, 0.35, 0.10}},
}
local function kitchen_material(name)
    return named[name] or palette[hash_string(name) % #palette + 1]
end

-- The model is in meters; the scene is in centimeters
add(model{file = "models/new+kitchen.obj", parts = kitchen_material, scale = 100})

-- Daylight outside the windows in the back wall
lights.panel({-120, 0, -350}, {480, 0, 0}, {0, 320, 0}, {5.0, 5.3, 6.0}, 3)

-- Three pendant lamps hanging over the island, each a bulb on a thin cord
local cord = lambertian{albedo = 0.02}
for _, x in ipairs{30, 140, 250} do
    lights.bulb({x, 190, -48}, 9, {70, 52, 32})
    add(box{min = {x - 0.4, 199, -48.4}, max = {x + 0.4, 300, -47.6}, material = cord})
end

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = 0.7,
    field_of_view = 58,
    look_from = {-170, 150, 215},
    look_at = {125, 105, -110},
}
