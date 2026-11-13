--[[
A snowy Christmas night: a decorated tree with presents, a snowman, Rudolph, lamp posts and
falling snow under the moon, using the CC0 Kenney Holiday Kit models
]]

local effects = require "effects"
local lights  = require "lights"

output "christmas.png"

local models   = "models/christmas/"
local colormap = image_texture{file = models .. "colormap.png"}
local painted  = lambertian{albedo = colormap} -- the Kenney models' colors

-- A Kenney model, `height` tall at `base`, turned `turn` degrees (0 faces the camera). The models
-- face +z, away from the camera, so they are turned around first.
local function place(name, height, base, turn)
    return add(model{file = models .. name .. ".obj", loader = "textured", material = painted,
                     height = height, base = base, turn_y = 180 + (turn or 0)})
end

--[[
The ground and the moon
]]
add(quad{corner = {-4000, 0, -2000}, u = {8000, 0, 0}, v = {0, 0, 6000},
         material = lambertian{albedo = {0.88, 0.92, 1.0}}})

-- Moonlight from behind the camera; the shadow rays aim at it 4 times as often as at a bulb
lights.bulb({-700, 1000, -1000}, 120, {40, 45, 60}, 4)

-- The moon itself: dim, and no shadow rays aim at it, so it adds no noise
lights.bulb({640, 740, 1650}, 85, {3.5, 3.8, 4.5}, 0)

--[[
The tree, with string lights spiraling up its tapering outline; shadow rays aim at every fourth
]]
local tree_height = 300
place("tree-decorated-snow", tree_height, {0, 0, 0})

local bulb_colors = {{7, 4.2, 1.4}, {7, 0.7, 0.7}, {0.8, 5.5, 1.8}, {1.2, 2.2, 8}}
local bulb_count  = 46
for i = 0, bulb_count - 1 do
    local fraction = 0.14 + 0.72 * i / (bulb_count - 1)
    local radius   = tree_height * (0.33 * (1 - fraction) + 0.012)
    local angle    = 2.4 * i
    lights.bulb({radius * math.cos(angle), tree_height * fraction, radius * math.sin(angle)}, 3,
                bulb_colors[i % 4 + 1], i % 4 == 0 and 1 or 0)
end

--[[
Presents under the tree
]]
place("present-a-cube", 56, {-95, 0, -70}, 15)
place("present-b-cube", 46, {70, 0, -85}, -20)
place("present-a-round", 38, {-45, 0, -118}, 35)
place("present-b-round", 62, {118, 0, -30}, 5)
place("present-b-cube", 30, {28, 0, -128}, 40)
place("present-a-cube", 72, {-135, 0, 15}, -10)

-- A gold-foil present with a red ribbon and bow, made from boxes and spheres
do
    local foil   = metal{albedo = {0.95, 0.72, 0.30}, fuzz = 0.04}
    local ribbon = lambertian{albedo = {0.75, 0.05, 0.07}}
    local size, strip = 44, 4
    local half = size / 2
    local c = vec(-10, 0, -72)
    add(box{min = {c.x - half, 0, c.z - half}, max = {c.x + half, size, c.z + half}, material = foil})
    add(box{min = {c.x - strip, 0, c.z - half - 0.5}, max = {c.x + strip, size + 0.5, c.z + half + 0.5},
            material = ribbon})
    add(box{min = {c.x - half - 0.5, 0, c.z - strip}, max = {c.x + half + 0.5, size + 0.5, c.z + strip},
            material = ribbon})
    add(sphere{center = {c.x - 7, size + 5, c.z}, radius = 7.5, material = ribbon})
    add(sphere{center = {c.x + 7, size + 5, c.z}, radius = 7.5, material = ribbon})
end

-- Chrome baubles that fell into the snow, reflecting the string lights
for _, bauble in ipairs{
    {center = {-58, 10, -165}, radius = 10, color = {0.88, 0.07, 0.07}},
    {center = {62, 8, -158}, radius = 8, color = {0.95, 0.72, 0.30}},
    {center = {-150, 9, -120}, radius = 9, color = {0.90, 0.90, 0.94}},
    {center = {150, 8, -95}, radius = 8, color = {0.10, 0.25, 0.85}},
} do
    add(sphere{center = bauble.center, radius = bauble.radius,
               material = metal{albedo = bauble.color, fuzz = 0.01}})
end

--[[
Characters and props
]]
place("snowman-hat", 125, {215, 0, -85}, 40)
place("nutcracker", 105, {-120, 0, 70}, -15)
place("gingerbread-man", 42, {48, 0, -178}, 10)
place("gingerbread-woman", 42, {82, 0, -172}, -15)
place("candy-cane-red", 62, {165, 0, -35}, 20)
place("candy-cane-green", 58, {-160, 0, -45}, -25)
place("sled", 42, {-235, 0, 25}, -30)

-- Rudolph: a reindeer whose nose is a red light, just in front of its frontmost point
do
    local turn     = -35
    local reindeer = place("reindeer", 125, {-185, 0, -85}, turn)
    local facing   = vec(-math.sin(math.rad(turn)), 0, -math.cos(math.rad(turn)))
    lights.bulb(reindeer:frontmost(facing) + 2.5 * facing, 3.6, {7, 0.2, 0.2})
end

--[[
Two lamp posts whose orange-painted glass glows, each with a light inside for the shadow rays
and a ball of mist for a halo
]]
for _, x in ipairs{320, -320} do
    local height = 190
    local glow   = diffuse_light{emission = {16, 11, 4}}
    add(model{file = models .. "lantern.obj", loader = "textured", material = painted,
              height = height, base = {x, 0, 120},
              face_materials = {
                  texture = colormap,
                  pick = function(color)
                      if color.x > 0.9 and color.y > 0.55 and color.z < 0.35 then return glow end
                  end,
              }})
    local glass_center = vec(x, 0.815 * height, 120)
    light(sphere{center = glass_center, radius = 6}, 2)
    effects.halo(glass_center, 34, 0.008, {1.0, 0.85, 0.6})
end

--[[
The woods behind
]]
for _, tree in ipairs{
    {"tree-snow-a", -330, 190, 300}, {"tree", -190, 330, 360}, {"tree-snow-c", -40, 420, 330},
    {"tree-snow-b", 120, 300, 290}, {"tree-snow-a", 300, 210, 330}, {"tree", 430, 360, 380},
    {"tree-snow-b", -470, 380, 400}, {"tree-snow-c", 560, 140, 280}, {"tree", -540, 120, 300},
} do
    local name, x, z, height = table.unpack(tree)
    place(name, height, {x, 0, z})
end
place("rocks-medium", 70, {-380, 0, 90}, 20)
place("snow-pile", 34, {-250, 0, 120})
place("snow-pile", 30, {225, 0, 150}, 90)
place("snow-pile", 28, {330, 0, -60}, 30)

--[[
Falling snow
]]
effects.snowfall{count = 650, min = {-460, 6, -300}, max = {460, 420, 420}, radius = {1.0, 2.6}}

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = {0.012, 0.020, 0.050}, -- night sky
    field_of_view = 40,
    look_from = {45, 85, -560},
    look_at = {0, 135, 0},
    defocus_angle = 0.45, -- shallow depth of field: far lights melt into bokeh
    focus_distance = 500,
}
