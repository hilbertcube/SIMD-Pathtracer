#pragma once

#include <cmath>

#include "rt/materials/material.h"

namespace rt
{

/*
 * Cloth like velvet or satin: a diffuse surface whose color shifts toward a sheen color at
 * grazing angles, where fibers standing up from the surface catch the light.
 */
class velvet : public material
{
public:
    // The sheen's strength is strength * (1 - cos(angle to the normal))^power
    velvet(const color& albedo, const color& sheen, float power = 3.0f, float strength = 1.0f)
        : albedo(albedo), sheen(sheen), power(power), strength(strength)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const float cosine =
            std::fmax(0.0f, dot(-unit_vector(incoming.direction()), record.normal));
        const float weight = std::fmin(1.0f, strength * std::pow(1.0f - cosine, power));

        scattering.attenuation   = (1.0f - weight) * albedo + weight * sheen;
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const float projection = dot(record.normal, scattered.direction());
        return projection <= 0.0f ? 0.0f : projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return false; }

private:
    color albedo, sheen;
    float power, strength;
};

} // namespace rt
