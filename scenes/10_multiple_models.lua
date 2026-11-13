-- The Cornell box with Suzanne and the teapot on two turned cubes and the cow on the floor

local cornell = require "cornell"

output "multiple_3d_models.png"

cornell.walls()

-- A cube standing on the floor, turned by `degrees`; returns the middle of its top face
local function add_cube(center_x, center_z, size, degrees)
    local half = size / 2
    local cube = box{min = {-half, 0, -half}, max = {half, size, half}, material = cornell.white}
    add(translate(rotate_y(cube, degrees), {center_x, 0, center_z}))
    return vec(center_x, size, center_z)
end

add(model{file = "models/suzanne.obj", material = lambertian{albedo = {0.9, 0.8, 0}},
          height = 95, base = add_cube(385, 355, 165, 15), turn_y = 200})
add(model{file = "models/teapot.obj", material = lambertian{albedo = 0.8},
          height = 90, base = add_cube(185, 170, 165, -18), turn_y = 20})
add(model{file = "models/spot.obj", material = lambertian{albedo = {0, 0.8, 0.9}},
          height = 100, base = {420, 0, 70}, turn_y = -35})

cornell.camera{image_width = 600, samples_per_pixel = 1000}
