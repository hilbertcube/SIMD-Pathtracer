-- Two large checkered spheres, one on top of the other

output "checked_spheres.png"

local checks = checker_texture{scale = 0.32, even = {0.2, 0.3, 0.1}, odd = 0.9}
add(sphere{center = {0, -10, 0}, radius = 10, material = lambertian{albedo = checks}})
add(sphere{center = {0, 10, 0}, radius = 10, material = lambertian{albedo = checks}})

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
