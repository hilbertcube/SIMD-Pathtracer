#pragma once

#include "rt/materials/media/isotropic.h"

namespace rt
{

/*
 * A scattering medium that also glows: fire, embers, a lamp's haze. The light is emitted where
 * the medium scatters, so a denser or thicker medium glows brighter. Use it as the phase
 * function of a constant_medium.
 */
class glowing_medium : public isotropic
{
public:
    glowing_medium(const color& albedo, const color& emission)
        : isotropic(albedo), emission(emission)
    {
    }

    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        return emission;
    }

private:
    color emission;
};

} // namespace rt
