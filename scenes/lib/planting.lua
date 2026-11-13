--[[
Planting copies of a few shapes (trees, say) on a jittered grid. The layout comes from a hash of
each grid position, so it stays the same whatever else the scene draws.
]]

local planting = {}

-- A fixed pseudo-random number in [0, 1) for three whole numbers (32-bit arithmetic)
function planting.hash(a, b, c)
    local h = ((a * 73856093) ~ (b * 19349663) ~ (c * 83492791)) & 0xffffffff
    h = h ~ (h >> 13)
    h = (h * 0x5bd1e995) & 0xffffffff
    h = h ~ (h >> 15)
    return (h & 0xffffff) / 0x1000000
end

-- Whole-number division that rounds toward zero, like C++'s
local function divide(a, b)
    local q = a // b
    if q < 0 and q * b ~= a then q = q + 1 end
    return q
end

--[[
Copies of `kinds` on a jittered grid `spacing` apart, turned at random, between the square radii
`inner` and `outer` and where `allowed(x, z)` agrees. Some squares of cells become clearings, and
kinds grow in patches; the settings and their defaults are below.
]]
function planting.grid(settings)
    local hash = planting.hash
    local kinds, spacing, seed = settings.kinds, settings.spacing, settings.seed or 0
    local inner, outer = settings.inner or 0, settings.outer
    local jitter = settings.jitter or 0.8
    local clearing_cell = settings.clearing_cell or 6
    local clearing_chance = settings.clearing_chance or 0.18
    local clearing_thinning = settings.clearing_thinning or 0.85
    local patch_cell, patch_share = settings.patch_cell or 5, settings.patch_share or 0.7

    local cells = math.floor(outer / spacing)
    for a = -cells, cells do
        for b = -cells, cells do
            local x = (a + jitter * (hash(a, b, seed + 1) - 0.5)) * spacing
            local z = (b + jitter * (hash(a, b, seed + 2) - 0.5)) * spacing
            local r = math.max(math.abs(x), math.abs(z))
            local keep = r >= inner and r <= outer and (not settings.allowed or settings.allowed(x, z))
            local clearing = hash(divide(a, clearing_cell), divide(b, clearing_cell), seed + 3) < clearing_chance
            if keep and not (clearing and hash(a, b, seed + 4) < clearing_thinning) then
                local patch = hash(a // patch_cell, b // patch_cell, seed + 7)
                local pick  = hash(a, b, seed + 8) < patch_share and patch or hash(a, b, seed + 5)
                local kind  = math.min(#kinds, math.floor(#kinds * pick) + 1)
                add(translate(rotate_y(kinds[kind], 360 * hash(a, b, seed + 6)), {x, 0, z}))
            end
        end
    end
end

return planting
