-- Spheres with a marble (Perlin noise) texture

output "perlin_spheres.png"

local marble = lambertian{albedo = noise_texture{scale = 4}}
add(sphere{center = {0, -1000, 0}, radius = 1000, material = marble})
add(sphere{center = {0, 2, 0}, radius = 2, material = marble})

camera{
    aspect_ratio = "16:9",
    image_width = 1000,
    samples_per_pixel = 100,
    max_depth = 50,
    background = {0.70, 0.80, 1.00},
    field_of_view = 20,
    look_from = {13, 2, 3},
    look_at = {0, 0, 0},
}
