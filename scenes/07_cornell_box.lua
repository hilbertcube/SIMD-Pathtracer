-- The Cornell box with two turned boxes, a sphere and a pyramid

local cornell = require "cornell"

output "cornell_box.png"

cornell.walls()

local tall_box = box{min = {0, 0, 0}, max = {165, 330, 165}, material = cornell.white}
add(translate(rotate_y(tall_box, 15), {265, 0, 295}))
local short_box = box{min = {0, 0, 0}, max = {165, 165, 165}, material = cornell.white}
add(translate(rotate_y(short_box, -18), {130, 0, 65}))

add(sphere{center = {200, 215, 200}, radius = 50, material = lambertian{albedo = {0.4, 0.2, 0.1}}})
add(pyramid{base_min = {325, 0, 100}, base_max = {475, 0, 200}, apex = {400, 150, 150},
            material = lambertian{albedo = {0.12, 0.45, 0.5}}})

cornell.camera{image_width = 600, samples_per_pixel = 100}
