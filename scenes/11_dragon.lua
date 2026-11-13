-- A gold dragon (250k triangles) on a stone pedestal over a mirror floor, before a red wall

local lights = require "lights"

output "dragon.png"

-- A dark mirror floor, so the dragon is reflected below itself
add(quad{corner = {-2000, 0, -1400}, u = {4000, 0, 0}, v = {0, 0, 2800},
         material = metal{albedo = {0.55, 0.55, 0.60}, fuzz = 0}})

-- A backdrop in the Cornell box's red
add(quad{corner = {-2000, 0, 700}, u = {4000, 0, 0}, v = {0, 1000, 0},
         material = lambertian{albedo = {0.65, 0.05, 0.05}}})

-- A softbox overhead, side strips for long reflections on the gold, a fill behind the camera
lights.panel({-600, 700, -300}, {1200, 0, 0}, {0, 0, 600}, 1.9, 3)
lights.panel({-820, 20, -250}, {0, 420, 0}, {0, 0, 400}, {0.8, 0.8, 0.9})
lights.panel({820, 20, -250}, {0, 0, 400}, {0, 420, 0}, {0.8, 0.8, 0.9})
lights.panel({-600, 100, -1150}, {1200, 0, 0}, {0, 400, 0}, 0.5)

-- The dragon, turned a little toward the camera, on a pedestal
local pedestal_height = 50
add(box{min = {-320, 0, -190}, max = {320, pedestal_height, 190},
        material = coated{albedo = {0.55, 0.55, 0.58}}})
add(model{file = "models/dragon.obj",
          material = conductor{reflectance = {1.0, 0.78, 0.34}, roughness = 0.25},
          height = 280, base = {0, pedestal_height, 0}, turn_y = 25})

camera{
    aspect_ratio = "4:3",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = {0.02, 0.02, 0.03},
    field_of_view = 38,
    look_from = {0, 260, -1100},
    look_at = {0, 5, 0},
}
