-- The Cornell box with a blue metal teapot

local cornell = require "cornell"

output "3d_model.png"

cornell.walls(lambertian{albedo = {0.4, 0.6, 0.9}})

-- Scaled 80 times, then moved into the middle of the box
add(model{file = "models/teapot.obj", material = metal{albedo = {0, 0, 0.8}, fuzz = 0},
          scale = 80, offset = {278, 0, 278}})

cornell.camera{image_width = 1000, samples_per_pixel = 20}
