#pragma once

#include <cmath>

#include "rt/materials/material.h"

namespace rt
{

/*
 * Rough diffuse (Oren-Nayar): a surface made of many tiny facets, like clay, plaster, sand or the
 * moon. Unlike a Lambertian one it looks flat and bright when lit from the viewer's side, and
 * brightens toward its edges, since the facets facing the light are seen from any angle.
 */
class oren_nayar : public material
{
public:
    // sigma is the facets' angular spread in radians: 0 is Lambertian, 0.5 to 1 is very rough
    oren_nayar(const color& albedo, float sigma)
        : oren_nayar(make_shared<solid_color>(albedo), sigma)
    {
    }

    oren_nayar(shared_ptr<texture> albedo_texture, float sigma)
        : albedo_texture(albedo_texture), a(1.0f - 0.5f * sigma * sigma / (sigma * sigma + 0.33f)),
          b(0.45f * sigma * sigma / (sigma * sigma + 0.09f))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    // The BRDF times the cosine, divided by the albedo
    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const vec3f view             = -unit_vector(incoming.direction());
        const vec3f light            = unit_vector(scattered.direction());
        const float normal_dot_light = dot(record.normal, light);
        const float normal_dot_view  = dot(record.normal, view);
        if (normal_dot_light <= 0.0f || normal_dot_view <= 0.0f)
        {
            return 0.0f;
        }

        // How much the two directions agree around the normal, from their parts in the surface
        const float light_sine_squared =
            std::fmax(0.0f, 1.0f - normal_dot_light * normal_dot_light);
        const float view_sine_squared = std::fmax(0.0f, 1.0f - normal_dot_view * normal_dot_view);
        float azimuth_cosine          = 0.0f;
        if (light_sine_squared > 1e-6f && view_sine_squared > 1e-6f)
        {
            azimuth_cosine =
                std::fmax(0.0f, (dot(light, view) - normal_dot_light * normal_dot_view) /
                                    std::sqrt(light_sine_squared * view_sine_squared));
        }

        // sin(max angle) * tan(min angle)
        const float larger_cosine  = std::fmax(normal_dot_light, normal_dot_view);
        const float smaller_cosine = std::fmin(normal_dot_light, normal_dot_view);
        const float sine_tangent   = std::sqrt(1.0f - smaller_cosine * smaller_cosine) *
                                   std::sqrt(1.0f - larger_cosine * larger_cosine) / larger_cosine;

        return normal_dot_light / PI * (a + b * azimuth_cosine * sine_tangent);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
    float a, b;
};

} // namespace rt
