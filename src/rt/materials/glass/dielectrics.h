#pragma once

#include <cmath>

#include "rt/materials/material.h"

namespace rt
{

class dielectrics : public material
{
private:
    float refraction_index;

    static float reflectance(float cosine, float refraction_index)
    {
        // Schlick's approximation
        auto normal_reflectance = (1.0f - refraction_index) / (1 + refraction_index);
        normal_reflectance *= normal_reflectance;
        return normal_reflectance + (1.0f - normal_reflectance) * std::pow((1 - cosine), 5);
    }

public:
    dielectrics(float refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = color(1.0f, 1.0f, 1.0f);
        scattering.direction_pdf = nullptr;
        scattering.is_specular   = true;
        float refraction_ratio   = record.front_face ? (1.0f / refraction_index) : refraction_index;

        vec3f unit_direction = unit_vector(incoming.direction());
        float cos_theta      = std::fmin(dot(-unit_direction, record.normal), 1.0f);
        float sin_theta      = std::sqrt(1.0f - cos_theta * cos_theta);

        bool cannot_refract = (refraction_ratio * sin_theta > 1.0f);
        vec3f direction;

        direction = (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float())
                        ? reflect(unit_direction, record.normal)
                        : refract(unit_direction, record.normal, refraction_ratio);

        scattering.specular_ray = ray(record.point, direction, incoming.time());
        return true;
    }

    bool needs_uv() const override { return false; }
};

} // namespace rt
