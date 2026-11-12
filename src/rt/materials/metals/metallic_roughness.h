#pragma once

#include <cmath>

#include "rt/materials/material.h"
#include "rt/materials/metals/conductor.h"
#include "rt/materials/textures/texture.h"

namespace rt
{

/*
 * PBR metallic-roughness maps (base color, metallic, roughness). Metal texels are a GGX conductor
 * tinted by the base color; the rest are diffuse. The metallic map is thresholded rather than
 * blended, so scatter() and scattering_pdf() always agree on which surface was hit.
 */
class metallic_roughness : public material
{
public:
    metallic_roughness(shared_ptr<texture> base_color, shared_ptr<texture> metallic,
                       shared_ptr<texture> roughness)
        : base_color(base_color), metallic(metallic), roughness(roughness)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const color albedo = base_color->value(record.u, record.v, record.point);
        if (is_metal(record))
        {
            return conductor(albedo, roughness_at(record)).scatter(incoming, record, scattering);
        }
        scattering.attenuation   = albedo;
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        if (is_metal(record))
        {
            // The color only tints the attenuation, not the density
            return conductor(color(1, 1, 1), roughness_at(record))
                .scattering_pdf(incoming, record, scattered);
        }
        const float projection = dot(record.normal, scattered.direction());
        return projection <= 0.0f ? 0.0f : projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return true; }

private:
    shared_ptr<texture> base_color, metallic, roughness;

    // Undoes the loader's gamma 2.2 decoding: these maps hold data, not colors
    static float raw(float linear) { return std::pow(std::fmax(linear, 0.0f), 1.0f / 2.2f); }

    bool is_metal(const hit_record& record) const
    {
        return raw(metallic->value(record.u, record.v, record.point).r()) > 0.5f;
    }

    float roughness_at(const hit_record& record) const
    {
        return raw(roughness->value(record.u, record.v, record.point).r());
    }
};

} // namespace rt
