#pragma once

#include <cmath>

#include "rt/materials/common/fresnel.h"
#include "rt/materials/material.h"

namespace rt
{

/*
 * A pearly base under a thin transparent film, like mother of pearl, oil on water or a soap
 * film. The film reflects light that interferes with the light reflected from beneath it, and
 * the colors that survive depend on the viewing angle.
 */
class iridescent : public material
{
public:
    // thickness_nm is the film's thickness in nanometers (a few hundred), film_index its
    // refractive index, and coat_reflectance how strongly it reflects when viewed head on
    iridescent(const color& base_albedo, float thickness_nm = 480.0f, float film_index = 1.4f,
               float coat_reflectance = 0.3f)
        : base_albedo(base_albedo), thickness_nm(thickness_nm), film_index(film_index),
          coat_reflectance(coat_reflectance)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        const vec3f unit_direction = unit_vector(incoming.direction());
        const float cosine         = std::fmax(0.0f, dot(-unit_direction, record.normal));
        const float fresnel =
            coat_reflectance + (1.0f - coat_reflectance) * schlick_fifth_power(cosine);

        if (random_float() < fresnel)
        {
            // Light that bounced inside the film travels an extra 2 * n * d * cos(theta) before
            // rejoining the surface reflection, and each wavelength is reinforced or cancelled
            const float sin_inside     = std::sqrt(1.0f - cosine * cosine) / film_index;
            const float cos_inside     = std::sqrt(1.0f - sin_inside * sin_inside);
            const float path           = 2.0f * film_index * thickness_nm * cos_inside;
            const float wavelengths[3] = {650.0f, 550.0f, 450.0f};
            float tint[3];
            for (int channel = 0; channel < 3; ++channel)
            {
                tint[channel] = 1.0f + std::cos(2.0f * PI * path / wavelengths[channel]);
            }
            scattering.attenuation   = color(tint[0], tint[1], tint[2]);
            scattering.direction_pdf = nullptr;
            scattering.is_specular   = true;
            scattering.specular_ray =
                ray(record.point, reflect(unit_direction, record.normal), incoming.time());
            return true;
        }

        scattering.attenuation   = base_albedo;
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
    color base_albedo;
    float thickness_nm;
    float film_index;
    float coat_reflectance;
};

} // namespace rt
