-- A marble sphere on a marble ground, lit by a sphere light and a quad light

local lights = require "lights"

output "simple_light.png"

local marble = lambertian{albedo = noise_texture{scale = 4}}
add(sphere{center = {0, -1000, 0}, radius = 1000, material = marble})
add(sphere{center = {0, 2, 0}, radius = 2, material = marble})

local lamp = diffuse_light{emission = 4}
lights.bulb({0, 7, 0}, 2, lamp)
lights.panel({3, 1, -2}, {2, 0, 0}, {0, 2, 0}, lamp)

camera{
    aspect_ratio = "16:9",
    image_width = 1000,
    samples_per_pixel = 100,
    max_depth = 50,
    background = 0,
    field_of_view = 20,
    look_from = {26, 3, 6},
    look_at = {0, 2, 0},
}
