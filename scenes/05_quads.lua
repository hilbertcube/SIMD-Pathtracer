-- Five colored quads arranged around the view

output "quads.png"

local function panel(corner, u, v, color)
    add(quad{corner = corner, u = u, v = v, material = lambertian{albedo = color}})
end
panel({-3, -2, 5}, {0, 0, -4}, {0, 4, 0}, {1.0, 0.2, 0.2}) -- left, red
panel({-2, -2, 0}, {4, 0, 0}, {0, 4, 0}, {0.2, 1.0, 0.2})  -- back, green
panel({3, -2, 1}, {0, 0, 4}, {0, 4, 0}, {0.2, 0.2, 1.0})   -- right, blue
panel({-2, 3, 1}, {4, 0, 0}, {0, 0, 4}, {1.0, 0.5, 0.0})   -- upper, orange
panel({-2, -3, 5}, {4, 0, 0}, {0, 0, -4}, {0.2, 0.8, 0.8}) -- lower, teal

camera{
    aspect_ratio = "16:9",
    image_width = 1000,
    samples_per_pixel = 100,
    max_depth = 50,
    background = {0.70, 0.80, 1.00},
    field_of_view = 80,
    look_from = {0, 0, 9},
    look_at = {0, 0, 0},
}
