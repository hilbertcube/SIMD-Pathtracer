#pragma once

#include <cmath>

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * A diffuse base under a clear glossy coat: plastic, ceramic, glaze, paint. The coat reflects
 * with Schlick's Fresnel probability, which rises toward grazing angles; otherwise the light
 * reaches the diffuse base. Each outcome is its own kind of scatter, so lights are still sampled
 * directly on the diffuse part.
 */
class coated : public material
{
public:
    coated(const color& albedo, float coat_index = 1.5f)
        : coated(make_shared<solid_color>(albedo), coat_index)
    {
    }

    coated(shared_ptr<texture> albedo_texture, float coat_index = 1.5f)
        : albedo_texture(albedo_texture),
          normal_reflectance(((coat_index - 1.0f) / (coat_index + 1.0f)) *
                             ((coat_index - 1.0f) / (coat_index + 1.0f)))
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f unit_direction = unit_vector(incoming.direction());
        const float cosine         = std::fmax(0.0f, dot(-unit_direction, record.normal));
        const float fresnel =
            normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(cosine);

        if (random_float() < fresnel)
        {
            scattering.attenuation   = color(1.0f, 1.0f, 1.0f);
            scattering.direction_pdf = nullptr;
            scattering.is_specular   = true;
            scattering.specular_ray =
                ray(record.point, reflect(unit_direction, record.normal), incoming.time());
            return true;
        }

        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
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

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
    float normal_reflectance; // Fresnel reflectance of the coat at normal incidence
};

} // namespace rt
