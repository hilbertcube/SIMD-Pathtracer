-- Eighteen teapots on a staircase, one material each, from the book's originals to jade and lava

local effects = require "effects"
local lights  = require "lights"

output "material_showcase.png"

--[[
The stage: a glossy floor, a checkered backdrop for the glass to refract, and three steps
]]
add(quad{corner = {-2000, 0, -1400}, u = {4000, 0, 0}, v = {0, 0, 2800},
         material = coated{albedo = {0.04, 0.04, 0.05}}})

local checks = checker_texture{scale = 90, even = {0.50, 0.50, 0.53}, odd = {0.10, 0.10, 0.12}}
add(quad{corner = {-2000, 0, 780}, u = {4000, 0, 0}, v = {0, 1000, 0}, material = lambertian{albedo = checks}})

local step_height = 45
local stone = coated{albedo = {0.55, 0.55, 0.58}}
for step = 1, 3 do
    add(box{min = {-640, 0, 120 + 180 * (step - 1)}, max = {640, step * step_height, 300 + 180 * (step - 1)},
            material = stone})
end

-- A softbox overhead, side strips for long reflections on the metals, a fill behind the camera
lights.panel({-600, 700, -300}, {1200, 0, 0}, {0, 0, 1000}, 2.4, 3)
lights.panel({-820, 20, -250}, {0, 420, 0}, {0, 0, 400}, {1.5, 1.5, 1.7})
lights.panel({820, 20, -250}, {0, 0, 400}, {0, 420, 0}, {1.5, 1.5, 1.7})
lights.panel({-600, 100, -1150}, {1200, 0, 0}, {0, 400, 0}, 0.6)

--[[
The teapots: five to a row, 90 tall, smooth-shaded and turned 20 degrees
]]
local height = 90
local turn   = 20
local spout  = vec(math.cos(math.rad(turn)), 0, -math.sin(math.rad(turn))) -- where the spouts point

-- Slot i (0 to 4) of a row, counted from the left of the picture (the camera's right is -x)
local spacing = 210
local function front(i) return vec(2 * spacing - spacing * i, 0, -60) end
local function middle(i) return vec(2 * spacing - spacing * i, step_height, 210) end
local function top(i) return vec(2 * spacing - spacing * i, 2 * step_height, 390) end
local function peak(i) return vec(2 * spacing - spacing * i, 3 * step_height, 570) end

-- A smooth-shaded teapot in a slot
local function teapot(material, base)
    return add(model{file = "models/teapot.obj", loader = "smooth", material = material,
                     height = height, base = base, turn_y = turn})
end

local glass = dielectrics{refraction_index = 1.5}

--[[
Front row: the original materials, and a coated plastic
]]
-- 1. Matte diffuse, with a 3D checker pattern
teapot(lambertian{albedo = checker_texture{scale = 14, even = {0.90, 0.85, 0.70}, odd = {0.08, 0.30, 0.35}}},
       front(0))
-- 2. Fuzzy metal: a mirror blurred by random perturbation
teapot(metal{albedo = {0.70, 0.75, 0.80}, fuzz = 0.3}, front(1))
-- 3. Clear glass
teapot(glass, front(2))
-- 4. A lamp: an emissive teapot
teapot(diffuse_light{emission = {1.0, 0.85, 0.55}}, front(3))
-- 5. Coated plastic
teapot(coated{albedo = {0.75, 0.04, 0.05}}, front(4))

--[[
Middle row: glass and metals
]]
-- 6. Emerald glass that darkens where it is thick
teapot(tinted_dielectric{refraction_index = 1.5, tint = {0.15, 0.75, 0.35}, distance = 35}, middle(0))
-- 7. Rough copper (GGX)
teapot(conductor{reflectance = {0.95, 0.64, 0.54}, roughness = 0.35}, middle(1))
-- 8. Gold mirror, with Fresnel
teapot(conductor{reflectance = {1.0, 0.78, 0.34}, roughness = 0}, middle(2))
-- 9. Hammered silver: rough metal with small dents from bump mapping
teapot(bumped{base = conductor{reflectance = {0.95, 0.95, 0.97}, roughness = 0.18},
              feature_scale = 0.35, height = 0.9}, middle(3))
-- 10. Iridescent pearl
teapot(iridescent{albedo = {0.85, 0.85, 0.88}, thickness_nm = 520}, middle(4))

--[[
Third row: the unusual ones, and the original Perlin noise
]]
-- 11. Crystal that splits light into colors
teapot(dispersive_dielectric{red_index = 1.45, green_index = 1.52, blue_index = 1.62}, top(0))
-- 12. Velvet
teapot(velvet{albedo = {0.30, 0.02, 0.08}, sheen = {1.0, 0.55, 0.65}, power = 3, strength = 1}, top(1))
-- 13. Perlin noise: the book's marble texture, scaled up to a teapot
teapot(lambertian{albedo = noise_texture{scale = 0.18, frequency = 0.045}}, top(2))
-- 14. Frosted glass: glass with a rough surface
teapot(rough_dielectric{refraction_index = 1.5, roughness = 0.45}, top(3))
-- 15. Brushed steel: grooves running around the teapot, so highlights streak up and down it
teapot(brushed_metal{reflectance = {0.80, 0.82, 0.85}, roughness_along = 0.12, roughness_across = 0.55},
       top(4))

--[[
Top row: clay, and the two that steam
]]
-- 16. Terracotta clay: rough diffuse (Oren-Nayar)
teapot(oren_nayar{albedo = {0.80, 0.42, 0.26}, sigma = 0.8}, peak(1))
-- 17. Jade: glass around a scattering medium, as light behaves in translucent stone
do
    local jade = teapot(glass, peak(2))
    add(constant_medium{boundary = jade, density = 0.10, albedo = {0.35, 0.78, 0.50}})
    effects.steam(jade:frontmost(spout))
end
-- 18. Lava: glass filled with a glowing medium
do
    local lava = teapot(glass, peak(3))
    add(constant_medium{boundary = lava, density = 0.04,
                        phase = glowing_medium{albedo = {1.0, 0.45, 0.08}, emission = {1.8, 0.45, 0.04}}})
    effects.steam(lava:frontmost(spout))
end

camera{
    aspect_ratio = "16:9",
    image_width = 1280,
    samples_per_pixel = 2000,
    max_depth = 50,
    background = {0.02, 0.02, 0.03},
    field_of_view = 32,
    look_from = {0, 340, -1100},
    look_at = {0, 105, 230},
}
