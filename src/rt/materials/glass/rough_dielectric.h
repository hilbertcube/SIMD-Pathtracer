#pragma once

#include <cmath>

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"
#include "rt/math/ONB.h"

namespace rt
{

/*
 * Frosted glass: glass whose surface is microscopically rough, so it scatters the light it
 * reflects and refracts. Each ray picks a GGX microfacet normal and reflects or refracts about
 * it, with Schlick's Fresnel choosing between the two. Roughness 0 is clear glass.
 */
class rough_dielectric : public material
{
public:
    // roughness in [0, 1]; the GGX width alpha is its square
    rough_dielectric(float refraction_index, float roughness)
        : refraction_index(refraction_index),
          alpha_squared(roughness * roughness * roughness * roughness)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f view            = -unit_vector(incoming.direction());
        const float normal_dot_view = dot(view, record.normal);
        if (normal_dot_view <= 0.0f)
        {
            return false;
        }

        // A microfacet normal from the GGX distribution
        const float u = random_float();
        const float cos_theta =
            std::fmax(1e-4f, std::sqrt((1.0f - u) / (1.0f + (alpha_squared - 1.0f) * u)));
        const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
        const float phi       = 2.0f * PI * random_float();
        const vec3f half =
            ONB(record.normal)
                .transform(vec3f(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta));

        const float view_dot_half = dot(view, half);
        if (view_dot_half <= 0.0f)
        {
            return false;
        }

        const float ratio        = record.front_face ? 1.0f / refraction_index : refraction_index;
        const float sin_view     = std::sqrt(1.0f - std::fmin(view_dot_half, 1.0f) * view_dot_half);
        float normal_reflectance = (1.0f - ratio) / (1.0f + ratio);
        normal_reflectance *= normal_reflectance;
        const float fresnel =
            normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(view_dot_half);

        const bool reflects   = ratio * sin_view > 1.0f || fresnel > random_float();
        const vec3f direction = reflects ? reflect(-view, half) : refract(-view, half, ratio);

        // The new direction must stay on the right side of the surface
        const float normal_dot_light = dot(direction, record.normal);
        if (reflects ? normal_dot_light <= 0.0f : normal_dot_light >= 0.0f)
        {
            return false;
        }

        // Importance sampling by the microfacet normal leaves only the shadowing-masking term
        const float weight = ggx_masking(normal_dot_view, alpha_squared) *
                             ggx_masking(std::fabs(normal_dot_light), alpha_squared) *
                             view_dot_half / (normal_dot_view * cos_theta);
        scattering.attenuation   = color(weight, weight, weight);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        scattering.specular_ray  = ray(record.point, direction, incoming.time());
        return true;
    }

    bool needs_uv() const override { return false; }

private:
    float refraction_index;
    float alpha_squared;
};

} // namespace rt
