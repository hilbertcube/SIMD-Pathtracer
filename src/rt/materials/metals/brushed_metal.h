#pragma once

#include <cmath>

#include "rt/materials/common/anisotropic_ggx.h"
#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * Brushed metal: a conductor whose roughness depends on the direction, like steel or aluminum
 * with fine parallel grooves. Highlights stretch across the grooves into long streaks and bands.
 * The grooves run around `axis`, so with the axis along a turned object they are rings.
 */
class brushed_metal : public material
{
public:
    // Roughness (as for conductor) along the grooves, which is small, and across them, which is
    // larger and sets how long the streaks are
    brushed_metal(const color& reflectance, float roughness_along, float roughness_across,
                  const vec3f& axis = vec3f(0, 1, 0))
        : reflectance(reflectance), alpha_t(std::fmax(0.02f, roughness_along * roughness_along)),
          alpha_b(std::fmax(0.02f, roughness_across * roughness_across)), axis(unit_vector(axis))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f view   = -unit_vector(incoming.direction());
        const float cosine = dot(view, record.normal);
        if (cosine <= 0.0f)
        {
            return false;
        }

        const float edge         = schlick_fifth_power(cosine);
        scattering.attenuation   = reflectance + edge * (color(1.0f, 1.0f, 1.0f) - reflectance);
        scattering.is_specular   = false;
        scattering.direction_pdf = scattering.emplace<anisotropic_ggx_pdf>(
            anisotropic_frame(record.normal, axis), view, alpha_t, alpha_b);
        return true;
    }

    // The microfacet BRDF times the cosine, without the Fresnel color (that is the attenuation)
    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        const vec3f view             = -unit_vector(incoming.direction());
        const vec3f light            = unit_vector(scattered.direction());
        const float normal_dot_view  = dot(record.normal, view);
        const float normal_dot_light = dot(record.normal, light);
        if (normal_dot_view <= 0.0f || normal_dot_light <= 0.0f)
        {
            return 0.0f;
        }
        const anisotropic_frame frame(record.normal, axis);
        const vec3f half = unit_vector(light + view);
        return anisotropic_ggx_distribution(dot(half, frame.tangent), dot(half, frame.bitangent),
                                            dot(half, frame.normal), alpha_t, alpha_b) *
               anisotropic_ggx_masking(dot(light, frame.tangent), dot(light, frame.bitangent),
                                       normal_dot_light, alpha_t, alpha_b) *
               anisotropic_ggx_masking(dot(view, frame.tangent), dot(view, frame.bitangent),
                                       normal_dot_view, alpha_t, alpha_b) /
               (4.0f * normal_dot_view);
    }

    bool needs_uv() const override { return false; }

private:
    color reflectance;
    float alpha_t, alpha_b;
    vec3f axis;
};

} // namespace rt
