#pragma once

#include <algorithm>

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * Glass whose refractive index differs for red, green and blue (blue bends most, as in real
 * glass), so edges split into colors like a prism.
 *
 * A path can only follow one color at a time, so each path picks a color channel and carries only
 * that channel's light (weighted by 3, so the average over paths is unbiased). There is no
 * per-path state in the renderer, so the channel is kept in the ray's time: a camera ray's time
 * is a random number in [0, 1), whose third gives the channel. The first scatter here adds 1 to
 * the time to record that the weight has been applied. This means the material can't be combined
 * with motion blur.
 */
class dispersive_dielectric : public material
{
public:
    dispersive_dielectric(float red_index, float green_index, float blue_index)
        : indices{red_index, green_index, blue_index}
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const bool weighted = incoming.time() >= 1.0f;
        const float time    = weighted ? incoming.time() - 1.0f : incoming.time();
        const int channel   = std::min(2, static_cast<int>(time * 3.0f));

        scattering.attenuation = color(1.0f, 1.0f, 1.0f);
        if (!weighted)
        {
            scattering.attenuation = color(channel == 0 ? 3.0f : 0.0f, channel == 1 ? 3.0f : 0.0f,
                                           channel == 2 ? 3.0f : 0.0f);
        }

        const ray refracted      = dielectric_ray(incoming, record, indices[channel]);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(refracted.origin(), refracted.direction(), time + 1.0f);
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float indices[3];
};

} // namespace rt
