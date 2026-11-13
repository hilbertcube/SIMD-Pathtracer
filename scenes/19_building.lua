--[[
Three Art Deco towers (400k triangles) in a park of tens of thousands of trees, under a pink
sunset. About 2 m to the unit: the tallest tower is 115 units, the trees 10 to 14.
]]

local lights   = require "lights"
local planting = require "planting"

output "building.png"

-- Where things are, shared by the parts of the scene
local site = {
    plaza_x = 85, plaza_z = 35, -- half the plaza's size
    plaza_height = 0.2,
    path_width = 8, path_length = 400,
    look_from = vec(80, 48, 235), -- right of the path and raised, to see the towers' sides
    look_at = vec(-5, 55, 0),
}
site.path_start     = vec(0, 0.1, site.plaza_z) -- middle of the plaza's front edge
site.path_direction = vec(0, 0, 1)
site.path_across    = vec(-1, 0, 0)

-- The camera's view direction in the ground plane
local function flat_view()
    local view = site.look_at - site.look_from
    return unit_vector(vec(view.x, 0, view.z))
end

--[[
The towers
]]

-- One material from the model's PBR textures (4k copies of its 8k originals)
local function tower_material(code)
    local function map(kind)
        return image_texture{file = "models/Building/textures/4k/MAT " .. code .. "_" .. kind .. ".jpg"}
    end
    return metallic_roughness{base_color = map("Base_color"), metallic = map("Metallic"),
                              roughness = map("Roughness")}
end

-- The three towers, each from its own file, rearranged with the tallest (C) in the middle
local function add_towers()
    local stone_and_bronze = tower_material("FC01")
    local podium           = tower_material("FC08")
    local function pick(name) return name == "MAT_FC08" and podium or stone_and_bronze end

    -- Where each stands in the original model, and here
    for _, tower in ipairs{{"tower_c", -53.25, 0}, {"tower_b", 0, -50}, {"tower_a", 47.3, 50}} do
        local file, model_x, x = table.unpack(tower)
        add(translate(model{file = "models/Building/OBJ/" .. file .. ".obj", parts = pick},
                      {x - model_x, site.plaza_height, 0}))
    end
end

--[[
The ground
]]

-- A lawn (its texture repeating every 7.5 units), a stone plaza and a path out from it
local function add_ground()
    local size  = 14000
    local grass = tiled_texture{texture = image_texture{file = "texture/aerial_grass_rock_diff_2k.jpg"},
                                repeat_u = size / 7.5, repeat_v = size / 7.5}
    add(quad{corner = {-size / 2, 0, -size / 2}, u = {size, 0, 0}, v = {0, 0, size},
             material = lambertian{albedo = grass}})

    local stone = lambertian{albedo = {0.30, 0.29, 0.27}}
    add(box{min = {-site.plaza_x, 0, -site.plaza_z}, max = {site.plaza_x, site.plaza_height, site.plaza_z},
            material = stone})
    add(quad{corner = site.path_start - (site.path_width / 2) * site.path_across,
             u = site.path_length * site.path_direction, v = site.path_width * site.path_across,
             material = stone})
end

--[[
The park
]]

--[[
The five kinds of tree, each at its own height on the origin, with cut-out leaves and blossoms.
The pack has no cherry bark, so the cherry uses Bark.png.
]]
local function load_tree_kinds()
    local folder = "models/Trees/texture/"
    local function image(file) return image_texture{file = folder .. file} end
    local function card(color_file, mask_file)
        return cutout{base = lambertian{albedo = image(color_file)}, mask = folder .. mask_file}
    end
    local function plain(albedo) return lambertian{albedo = albedo} end

    local oak   = "oak_leaf.png"
    local mango = "pngtree-mango-leaves-isolated-bright-png-image_11272939-removebg-preview.png"
    local pale_bark   = lambertian{albedo = image("tree_bark_03_diff_2k.png")}
    local twig_leaves = card("Leaf.png", "Leaf.png")
    local twigs       = card("Branch.png", "Branch.png")

    -- The pack has no .mtl, so its material names are mapped by hand
    local materials = {
        ["Leaf"] = twig_leaves,
        ["Leaf.001"] = twig_leaves,
        ["Branch"] = twigs,
        ["Branch.001"] = twigs,
        ["Material.001"] = card(oak, oak),
        ["Material.006"] = card(mango, mango),
        ["Material.002"] = pale_bark,
        ["Material.005"] = pale_bark,
        ["v176CherryFlower.002"] = card("v176CherryFlower_BC.jpg", "v176CherryFlower_O.jpg"),
        ["v176CherryBud.001"] = plain({0.85, 0.45, 0.55}),
    }
    local bark = lambertian{albedo = image("Bark.png")} -- bark, knots, and the default
    local function pick(name) return materials[name] or bark end

    local kinds = {}
    for _, kind in ipairs{{"tree_a", 12}, {"tree_b", 11}, {"tree_c", 14}, {"tree_d", 10.5}, {"cherry", 10}} do
        local file, height = table.unpack(kind)
        kinds[#kinds + 1] = model{file = "models/Trees/OBJ/" .. file .. ".obj", parts = pick, height = height}
    end
    return kinds
end

-- Whether a tree at (x, z) would stand on the plaza or the path
local function on_paving(x, z)
    if math.abs(x) < site.plaza_x + 6 and math.abs(z) < site.plaza_z + 6 then
        return true
    end
    local offset = vec(x, 0, z) - site.path_start
    return dot(offset, site.path_direction) > 0 and
           math.abs(dot(offset, site.path_across)) < site.path_width / 2 + 4
end

-- Where the dense woods may grow: anywhere off the paving
local function off_paving(x, z)
    return not on_paving(x, z)
end

-- Where the distant woods may grow: off the paving and in the camera's view (0.75 radians)
local camera_view = flat_view()
local function in_view(x, z)
    local from_camera = unit_vector(vec(x - site.look_from.x, 0, z - site.look_from.z))
    return off_paving(x, z) and dot(from_camera, camera_view) >= math.cos(0.75)
end

-- Dense woods around the towers, then sparser ones (crowns merge at a distance) to the horizon
local function plant_trees()
    local kinds = load_tree_kinds()
    planting.grid{kinds = kinds, spacing = 11, outer = 500, seed = 0, allowed = off_paving}
    planting.grid{kinds = kinds, spacing = 24, inner = 500, outer = 5500, seed = 10, allowed = in_view}
end

--[[
The sky and the sun
]]

--[[
The Belfast Sunset sky (Poly Haven, CC0), dimmed and turned to put its afterglow behind the
towers. Its sun has set, so a separate rose-gold sun lights the scene.
]]
local function add_sky_and_sun()
    local view = flat_view()
    local left = vec(view.z, 0, -view.x)
    local function heading(radians_left) -- a horizontal direction, turned left from the view
        return math.cos(radians_left) * view + math.sin(radians_left) * left
    end

    local glow     = heading(-0.2)
    local sky_turn = 2 * math.pi * 0.605 - math.atan(glow.z, glow.x)
    local sky = sky_image{file = "texture/belfast_sunset_puresky_4k.hdr", strength = 0.25,
                          rotation_degrees = math.deg(sky_turn)}

    -- The sun, 32 degrees up, as bright as a clear day's
    local elevation = math.rad(32)
    lights.sun{direction = math.cos(elevation) * heading(2.5) + math.sin(elevation) * vec(0, 1, 0),
               color = {4.7, 4.1, 3.8}}
    return sky
end

add_towers()
add_ground()
plant_trees()

camera{
    aspect_ratio = "3:2",
    image_width = 1200,
    samples_per_pixel = 1000,
    max_depth = 16,
    sky = add_sky_and_sun(),
    field_of_view = 45,
    look_from = site.look_from,
    look_at = site.look_at,
}
