--[[
The final scene of Ray Tracing: The Next Week: a field of boxes, an area light, fog, glass,
metal, an earth globe, a marble sphere and a cluster of spheres
]]

output "final_scene.png"

-- A 20 x 20 field of boxes 100 wide, of random heights, in their own BVH
local ground = lambertian{albedo = {0.48, 0.83, 0.53}}
local boxes = {}
local box_width = 100
for i = 0, 19 do
    for j = 0, 19 do
        local min_x, min_z = -1000 + i * box_width, -1000 + j * box_width
        boxes[#boxes + 1] = box{min = {min_x, 0, min_z},
                                max = {min_x + box_width, random_float(1, 101), min_z + box_width},
                                material = ground}
    end
end
add(bvh_node(boxes))

light(add(quad{corner = {123, 554, 147}, u = {300, 0, 0}, v = {0, 0, 265},
               material = diffuse_light{emission = 7}}))

-- A sphere moving 30 to the right while the shutter is open
local center = vec(400, 400, 200)
add(sphere{center = center, center2 = center + vec(30, 0, 0), radius = 50,
           material = lambertian{albedo = {0.7, 0.3, 0.1}}})

local glass = dielectrics{refraction_index = 1.5}
add(sphere{center = {260, 150, 45}, radius = 50, material = glass})
add(sphere{center = {0, 150, 145}, radius = 50, material = metal{albedo = {0.8, 0.8, 0.9}, fuzz = 1}})

-- A glass ball filled with blue smoke, and thin mist over everything
local ball = add(sphere{center = {360, 150, 145}, radius = 70, material = glass})
add(constant_medium{boundary = ball, density = 0.2, albedo = {0.2, 0.4, 0.9}})
add(constant_medium{boundary = sphere{center = {0, 0, 0}, radius = 5000}, density = 0.0001, albedo = 1})

add(sphere{center = {400, 200, 400}, radius = 100,
           material = lambertian{albedo = image_texture{file = "texture/earth2048.bmp"}}})
add(sphere{center = {220, 280, 300}, radius = 80,
           material = lambertian{albedo = noise_texture{scale = 0.2}}})

-- A thousand spheres in a cube 165 on a side, turned and moved up as one
local white = lambertian{albedo = 0.73}
local cluster = {}
for i = 1, 1000 do
    cluster[i] = sphere{center = random_vector(0, 165), radius = 10, material = white}
end
add(translate(rotate_y(bvh_node(cluster), 15), {-100, 270, 395}))

camera{
    aspect_ratio = 1,
    image_width = 1000,
    samples_per_pixel = 2000,
    max_depth = 40,
    background = 0,
    field_of_view = 40,
    look_from = {478, 278, -600},
    look_at = {278, 278, 0},
}
