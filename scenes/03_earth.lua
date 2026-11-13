-- A globe with an earth texture

output "earth.png"

local earth = image_texture{file = "texture/earth2048.bmp"}
add(sphere{center = {0, 0, 0}, radius = 2, material = lambertian{albedo = earth}})

camera{
    aspect_ratio = "16:9",
    image_width = 1000,
    samples_per_pixel = 100,
    max_depth = 50,
    background = {0.70, 0.80, 1.00},
    field_of_view = 20,
    look_from = {0, 0, 12},
    look_at = {0, 0, 0},
}
