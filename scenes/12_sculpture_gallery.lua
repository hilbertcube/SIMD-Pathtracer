-- Six statues on marble pedestals under spotlights, in two staggered rows

local lights = require "lights"

output "sculpture_gallery.png"

local wall_paint = lambertian{albedo = {0.45, 0.12, 0.10}}
local ceiling    = lambertian{albedo = 0.75}
local floor      = lambertian{albedo = {0.30, 0.29, 0.28}} -- polished stone
local marble     = lambertian{albedo = {0.80, 0.78, 0.74}}
local spotlight  = diffuse_light{emission = {18, 16, 13}}

-- The room: x 0..900, y 0..500, z 0..600, open toward the camera
add(quad{corner = {0, 0, 0}, u = {900, 0, 0}, v = {0, 0, 600}, material = floor})
add(quad{corner = {0, 500, 0}, u = {900, 0, 0}, v = {0, 0, 600}, material = ceiling})
add(quad{corner = {0, 0, 600}, u = {900, 0, 0}, v = {0, 500, 0}, material = wall_paint})
add(quad{corner = {0, 0, 0}, u = {0, 500, 0}, v = {0, 0, 600}, material = wall_paint})
add(quad{corner = {900, 0, 0}, u = {0, 500, 0}, v = {0, 0, 600}, material = wall_paint})

local pedestal_height = 140

-- A pedestal at (x, z) with a spotlight above it; returns the middle of its top face
local function pedestal(x, z, width, depth)
    add(box{min = {x - width / 2, 0, z - depth / 2},
            max = {x + width / 2, pedestal_height, z + depth / 2}, material = marble})
    lights.panel({x - 45, 499, z - 45}, {90, 0, 0}, {0, 0, 90}, spotlight)
    return vec(x, pedestal_height, z)
end

local stone      = lambertian{albedo = {0.85, 0.83, 0.78}}
local gold       = metal{albedo = {0.95, 0.72, 0.30}, fuzz = 0.2}
local bronze     = metal{albedo = {0.55, 0.35, 0.20}, fuzz = 0.35}
local terracotta = lambertian{albedo = {0.76, 0.40, 0.26}}
local silver     = metal{albedo = {0.90, 0.90, 0.93}, fuzz = 0.03}
local jade       = lambertian{albedo = {0.25, 0.60, 0.42}}

-- Front row
add(model{file = "models/lucy.obj", material = stone, height = 230,
          base = pedestal(160, 280, 120, 120), tilt_x = -90})
add(model{file = "models/happy.obj", material = gold, height = 200,
          base = pedestal(410, 280, 120, 120)})
add(model{file = "models/max-planck.obj", material = bronze, height = 190,
          base = pedestal(630, 280, 120, 120)})

-- Back row, each statue between two in front (the long lion gets a long pedestal)
add(model{file = "models/demosthenes-low-res.obj", material = terracotta, height = 190,
          base = pedestal(265, 500, 120, 120), turn_y = 160})
add(model{file = "models/nefertiti.obj", material = silver, height = 200,
          base = pedestal(525, 500, 120, 140), turn_y = 180, tilt_x = -90})
add(model{file = "models/lionstatue.obj", material = jade, height = 100,
          base = pedestal(805, 495, 110, 170), turn_y = 210})

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = 0,
    field_of_view = 36,
    look_from = {450, 280, -700},
    look_at = {450, 210, 420},
}
