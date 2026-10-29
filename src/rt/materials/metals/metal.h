#pragma once

#include "rt/materials/material.h"

namespace rt
{

class metal : public material
{
private:
    color albedo;
    float fuzz;

public:
    metal(const color& albedo, float fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        vec3f reflected = reflect(incoming.direction(), record.normal);
        reflected       = unit_vector(reflected) + (fuzz * random_unit_vector<float, 3>());

        scattering.attenuation   = albedo;
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(record.point, reflected, incoming.time());

        // Absorb fuzzed reflections that end up below the surface
        return dot(reflected, record.normal) > 0;
    }

    bool needs_uv() const override { return false; }
};

} // namespace rt
