#pragma once

#include <cmath>

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * Glass that absorbs light as it travels through (Beer-Lambert), so thick parts are darker and
 * more saturated than thin ones. `tint` is the color of white light after it has crossed
 * `distance` units of the glass.
 */
class tinted_dielectric : public material
{
public:
    tinted_dielectric(float refraction_index, const color& tint, float distance)
        : refraction_index(refraction_index),
          absorption(-std::log(std::fmax(tint.r(), 1e-4f)) / distance,
                     -std::log(std::fmax(tint.g(), 1e-4f)) / distance,
                     -std::log(std::fmax(tint.b(), 1e-4f)) / distance)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation = color(1.0f, 1.0f, 1.0f);
        if (!record.front_face)
        {
            // The ray reached this surface from inside the glass, over `distance` units
            const float distance = record.t * incoming.direction().length();
            scattering.attenuation =
                color(std::exp(-absorption.r() * distance), std::exp(-absorption.g() * distance),
                      std::exp(-absorption.b() * distance));
        }
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = dielectric_ray(incoming, record, refraction_index);
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float refraction_index;
    color absorption; // per unit distance, for red, green and blue
};

} // namespace rt
