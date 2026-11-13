-- Ray Tracing in One Weekend's final scene: many small random spheres around three large ones

output "spheres.png"

local ground = checker_texture{scale = 0.32, even = 0.5, odd = 0.9}
add(sphere{center = {0, -1000, 0}, radius = 1000, material = lambertian{albedo = ground}})

-- A small sphere per grid cell, clear of the big metal one: 80% diffuse, 15% metal, 5% glass
for grid_x = -11, 10 do
    for grid_z = -11, 10 do
        local choice = random_float()
        local center = vec(grid_x + 0.9 * random_float(), 0.2, grid_z + 0.9 * random_float())
        if (center - vec(4, 0.2, 0)):length() > 0.9 then
            local material
            if choice < 0.8 then
                material = lambertian{albedo = random_vector() * random_vector()}
            elseif choice < 0.95 then
                material = metal{albedo = random_vector(0.5, 1), fuzz = random_float(0.5, 1)}
            else
                material = dielectrics{refraction_index = 1.5}
            end
            add(sphere{center = center, radius = 0.2, material = material})
        end
    end
end

-- The three big spheres
add(sphere{center = {0, 1, 0}, radius = 1, material = dielectrics{refraction_index = 1.5}})
add(sphere{center = {-4, 1, 0}, radius = 1, material = lambertian{albedo = {0.4, 0.2, 0.1}}})
add(sphere{center = {4, 1, 0}, radius = 1, material = metal{albedo = {0.7, 0.6, 0.5}, fuzz = 0}})

camera{
    aspect_ratio = "16:9",
    image_width = 1000,
    samples_per_pixel = 100,
    max_depth = 50,
    background = {0.70, 0.80, 1.00},
    field_of_view = 20,
    look_from = {13, 2, 3},
    look_at = {0, 0, 0},
    defocus_angle = 0.6,
    focus_distance = 10,
}
