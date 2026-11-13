-- A 1969-style Mustang (360k polygons) in a dark studio, with a material for each part by name

local lights = require "lights"

output "mustang.png"

local paint  = {0.85, 0.20, 0.02} -- Competition Orange
local chrome = {0.95, 0.95, 0.97}

-- Each part's look, by the part names in the model. Its own material file only lists the names.
local function mustang_material(name)
    if name == "M_Paintjob1" or name == "M_PaintJob2" then
        -- The body: a black-and-white stripe image, its white parts taking the paint's color
        local stripes = name == "M_Paintjob1" and "MustangBodyStripe1.png" or "MustangBodyStripe2.png"
        return coated{albedo = image_texture{file = "models/Mustang/" .. stripes, tint = paint},
                      coat_index = 1.6}
    elseif name == "M_ChromeTrim" or name == "M_ChromeTrim2" or name == "M_Mirror1" then
        return conductor{reflectance = chrome, roughness = 0}
    elseif name == "M_GrillTrim2" or name == "M_HeadLight1" then
        return conductor{reflectance = chrome, roughness = 0.05}
    elseif name == "M_Rim1" or name == "M_Rim2" then
        return conductor{reflectance = {0.80, 0.81, 0.84}, roughness = 0.18}
    elseif name == "M_Tire1" then
        return oren_nayar{albedo = {0.025, 0.025, 0.03}, sigma = 0.6}
    elseif name == "M_BrakePad1" or name == "M_BrakePad2" then
        return conductor{reflectance = {0.45, 0.45, 0.48}, roughness = 0.45}
    elseif name == "M_Exhaust" then
        return conductor{reflectance = {0.55, 0.55, 0.58}, roughness = 0.3}
    elseif name == "M_Glass1" or name == "M_WindShield" or name == "M_HeadLight2" then
        return tinted_dielectric{refraction_index = 1.5, tint = {0.55, 0.62, 0.60}, distance = 40}
    elseif name == "M_Tailight1" or name == "M_Tailight2" then
        return coated{albedo = {0.65, 0.02, 0.02}}
    elseif name == "M_TurnSignalLight1" then
        return coated{albedo = {0.9, 0.45, 0.05}}
    end
    -- Black trim, the grille, the underside: dark plastic and rubber
    return coated{albedo = {0.03, 0.03, 0.035}}
end

--[[
The model is in meters, facing +x, its wheels at y = -0.36 and its middle near (-0.55, 0, 0.12).
The scene is in centimeters, with the wheels on the floor.
]]
add(model{file = "models/Mustang/Mustang.obj", parts = mustang_material, scale = 100,
          offset = {55, 36, -12}})

-- A glossy dark floor, so the car is reflected in it
add(quad{corner = {-4000, 0, -4000}, u = {8000, 0, 0}, v = {0, 0, 8000},
         material = coated{albedo = {0.05, 0.05, 0.06}}})

-- A softbox overhead, and two behind the camera for the front, the side and long highlights
lights.panel({-600, 500, -500}, {1200, 0, 0}, {0, 0, 1000}, {3.0, 3.0, 3.2}, 3)
lights.panel({-100, 450, -1000}, {800, 0, 0}, {0, 0, 600}, {3.0, 3.0, 3.4}, 2)
lights.panel({900, 20, -1000}, {0, 0, 500}, {0, 300, 0}, {3.0, 3.0, 3.4}, 2)

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 1000,
    max_depth = 50,
    background = {0.01, 0.01, 0.015},
    field_of_view = 34,
    look_from = {560, 140, -620},
    look_at = {0, 60, 0},
}
