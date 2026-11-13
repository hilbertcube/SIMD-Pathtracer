-- Mist, steam and snow

local effects = {}

-- A ball of mist around `center`, so a lamp there gets a soft halo
function effects.halo(center, radius, density, color)
    return add(constant_medium{boundary = sphere{center = center, radius = radius}, density = density,
                               albedo = color})
end

-- Steam rising from `tip`: puffs of forward-scattering mist that widen and thin as they rise
function effects.steam(tip)
    local steam = mist{albedo = {0.95, 0.95, 1.0}, g = 0.6}
    local puff_count = 22
    for i = 0, puff_count - 1 do
        local rise   = i / (puff_count - 1)
        local drift  = 2 + 26 * rise
        local center = tip + vec(random_float(-drift, drift), 3 + 125 * rise, random_float(-drift, drift))
        local radius = 5 + 16 * rise + random_float(0, 3)
        add(constant_medium{boundary = sphere{center = center, radius = radius},
                            density = 0.22 * (1 - 0.92 * rise), phase = steam})
    end
end

-- `count` snowflakes at random between `min` and `max`, radii between radius[1] and radius[2]
function effects.snowfall(settings)
    local low, high = settings.min, settings.max
    local material = settings.material or lambertian{albedo = {0.95, 0.95, 1.0}}
    for _ = 1, settings.count do
        local position = vec(random_float(low[1], high[1]), random_float(low[2], high[2]),
                             random_float(low[3], high[3]))
        add(sphere{center = position, radius = random_float(settings.radius[1], settings.radius[2]),
                   material = material})
    end
end

return effects
