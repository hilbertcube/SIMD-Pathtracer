#pragma once

#include <cmath>

#include "rt/geometry/hittable.h"
#include "rt/math/random.h"
#include "rt/math/ray.h"
#include "rt/math/vector.h"

namespace rt
{

// (1 - cosine)^5, the angular part of Schlick's approximation of the Fresnel reflectance
inline float schlick_fifth_power(float cosine)
{
    const float m  = 1.0f - cosine;
    const float m2 = m * m;
    return m2 * m2 * m;
}

/*
 * The ray after hitting a glass-like surface: it reflects or refracts, with the reflection chosen
 * by Schlick's Fresnel probability (or always, past the critical angle).
 */
inline ray dielectric_ray(const ray& incoming, const hit_record& record, float refraction_index)
{
    const float ratio          = record.front_face ? 1.0f / refraction_index : refraction_index;
    const vec3f unit_direction = unit_vector(incoming.direction());
    const float cos_theta      = std::fmin(dot(-unit_direction, record.normal), 1.0f);
    const float sin_theta      = std::sqrt(1.0f - cos_theta * cos_theta);

    float normal_reflectance = (1.0f - ratio) / (1.0f + ratio);
    normal_reflectance *= normal_reflectance;
    const float reflectance =
        normal_reflectance + (1.0f - normal_reflectance) * schlick_fifth_power(cos_theta);

    const bool reflects   = ratio * sin_theta > 1.0f || reflectance > random_float();
    const vec3f direction = reflects ? reflect(unit_direction, record.normal)
                                     : refract(unit_direction, record.normal, ratio);
    return ray(record.point, direction, incoming.time());
}

} // namespace rt
