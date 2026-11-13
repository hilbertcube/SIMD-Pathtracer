-- A ring of colorful bunnies around a glowing orb at night, with a horse watching from the dark

local lights = require "lights"

output "bunny_campfire.png"

local ground = checker_texture{scale = 40, even = {0.10, 0.16, 0.07}, odd = {0.05, 0.09, 0.04}}
add(quad{corner = {-3000, 0, -3000}, u = {6000, 0, 0}, v = {0, 0, 6000},
         material = lambertian{albedo = ground}})

-- The fire: a glowing orb with a glass shell
local fire_center = vec(0, 45, 0)
lights.bulb(fire_center, 18, {34, 14, 3.5})
add(sphere{center = fire_center, radius = 26, material = dielectrics{refraction_index = 1.3}})

-- Bunnies evenly spaced on a circle, each turned to face the fire (the model faces -x)
local colors = {{0.85, 0.20, 0.20}, {0.95, 0.60, 0.15}, {0.90, 0.85, 0.25}, {0.30, 0.75, 0.30},
                {0.25, 0.55, 0.90}, {0.55, 0.35, 0.85}, {0.92, 0.92, 0.92}}
local ring_radius = 160
for i, color in ipairs(colors) do
    local angle = 2 * math.pi * (i - 0.5) / #colors
    add(model{file = "models/stanford-bunny.obj", material = lambertian{albedo = color}, height = 70,
              base = {ring_radius * math.sin(angle), 0, ring_radius * math.cos(angle)},
              turn_y = math.deg(angle) - 90})
end

add(model{file = "models/horse.obj", material = lambertian{albedo = {0.45, 0.30, 0.18}},
          height = 190, base = {-260, 0, 330}, turn_y = -30, tilt_x = -90})

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = {0.02, 0.025, 0.06}, -- moonlit night sky
    field_of_view = 36,
    look_from = {0, 230, -560},
    look_at = {0, 40, 40},
}
