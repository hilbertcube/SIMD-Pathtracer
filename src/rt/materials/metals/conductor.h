#pragma once

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * A conductor (metal) with GGX microfacet roughness and Fresnel reflection: the color is the
 * reflectance at normal incidence, and it turns whiter toward grazing angles. Roughness 0 is a
 * perfect mirror. Rough metals are sampled like diffuse surfaces, so lights are sampled directly
 * and highlights converge much faster than with a fuzzy mirror.
 */
class conductor : public material
{
public:
    // roughness in [0, 1]; the GGX width alpha is its square
    conductor(const color& reflectance, float roughness)
        : reflectance(reflectance), alpha(roughness * roughness)
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

        const float edge       = schlick_fifth_power(cosine);
        scattering.attenuation = reflectance + edge * (color(1.0f, 1.0f, 1.0f) - reflectance);
        scattering.is_specular = alpha < 1e-3f;
        if (scattering.is_specular)
        {
            scattering.direction_pdf = nullptr;
            scattering.specular_ray =
                ray(record.point, reflect(-view, record.normal), incoming.time());
        }
        else
        {
            scattering.direction_pdf = scattering.emplace<ggx_pdf>(record.normal, view, alpha);
        }
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
        const float alpha_squared   = alpha * alpha;
        const float normal_dot_half = dot(record.normal, unit_vector(light + view));
        return ggx_distribution(normal_dot_half, alpha_squared) *
               ggx_masking(normal_dot_light, alpha_squared) *
               ggx_masking(normal_dot_view, alpha_squared) / (4.0f * normal_dot_view);
    }

    bool needs_uv() const override { return false; }

private:
    color reflectance;
    float alpha;
};

} // namespace rt
