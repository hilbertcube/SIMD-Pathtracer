-- A chrome armadillo and a glass bunny lit only by neon tubes, over a glossy black floor

local lights = require "lights"

output "neon_stage.png"

add(quad{corner = {-1000, 0, -1000}, u = {2000, 0, 0}, v = {0, 0, 1600},
         material = metal{albedo = {0.08, 0.08, 0.09}, fuzz = 0.03}})
add(quad{corner = {-1000, 0, 600}, u = {2000, 0, 0}, v = {0, 800, 0},
         material = lambertian{albedo = {0.05, 0.05, 0.06}}})

local magenta = diffuse_light{emission = {9, 0.6, 6}}
local cyan    = diffuse_light{emission = {0.4, 6, 9}}

-- Neon tubes on the back wall: thin panels facing the camera, alternating colors
for i = 0, 5 do
    lights.panel({-375 + 150 * i, 40, 590}, {0, 420, 0}, {8, 0, 0}, i % 2 == 0 and magenta or cyan)
end

-- Two panels standing on the floor at the sides, facing inward
lights.panel({-330, 0, 120}, {0, 300, 0}, {0, 0, 260}, cyan)
lights.panel({330, 0, 120}, {0, 0, 260}, {0, 300, 0}, magenta)

add(model{file = "models/armadillo.obj", material = metal{albedo = {0.92, 0.92, 0.95}, fuzz = 0.03},
          height = 300, base = {-40, 0, 260}, turn_y = 15})
add(model{file = "models/stanford-bunny.obj", material = dielectrics{refraction_index = 1.5},
          height = 110, base = {170, 0, 120}, turn_y = -60})

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = 0,
    field_of_view = 38,
    look_from = {0, 170, -700},
    look_at = {0, 150, 200},
}
