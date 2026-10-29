#pragma once

#include <cmath>

#include "rt/materials/common/pdf.h"
#include "rt/math/constants.h"
#include "rt/math/random.h"
#include "rt/math/vector.h"

namespace rt
{

// The GGX distribution stretched along the tangent and bitangent, with widths alpha_t and
// alpha_b. The arguments are the half vector's components along tangent, bitangent and normal.
inline float anisotropic_ggx_distribution(float half_t, float half_b, float half_n, float alpha_t,
                                          float alpha_b)
{
    const float sum = half_t * half_t / (alpha_t * alpha_t) +
                      half_b * half_b / (alpha_b * alpha_b) + half_n * half_n;
    return 1.0f / (PI * alpha_t * alpha_b * sum * sum);
}

// Smith masking for that distribution, for a direction with the given components
inline float anisotropic_ggx_masking(float direction_t, float direction_b, float direction_n,
                                     float alpha_t, float alpha_b)
{
    const float lambda = 0.5f * (std::sqrt(1.0f + (alpha_t * alpha_t * direction_t * direction_t +
                                                   alpha_b * alpha_b * direction_b * direction_b) /
                                                      (direction_n * direction_n)) -
                                 1.0f);
    return 1.0f / (1.0f + lambda);
}

// A surface frame for anisotropic materials, and the GGX sampling that goes with it
struct anisotropic_frame
{
    vec3f tangent, bitangent, normal;

    // The tangent runs around `axis`, as the grooves of a turned and brushed object do
    anisotropic_frame(const vec3f& normal, const vec3f& axis) : normal(normal)
    {
        vec3f around = cross(axis, normal);
        if (around.length_squared() < 1e-6f) // at the poles of the axis
        {
            around = cross(std::fabs(normal.x()) < 0.9f ? vec3f(1, 0, 0) : vec3f(0, 1, 0), normal);
        }
        tangent   = unit_vector(around);
        bitangent = cross(normal, tangent);
    }
};

/*
 * The directions a brushed conductor reflects into: pick a microfacet normal from the
 * anisotropic GGX distribution, then mirror the view direction about it.
 */
class anisotropic_ggx_pdf : public pdf
{
public:
    anisotropic_ggx_pdf(const anisotropic_frame& frame, const vec3f& view, float alpha_t,
                        float alpha_b)
        : frame(frame), view(view), alpha_t(alpha_t), alpha_b(alpha_b)
    {
    }

    float value(const vec3f& direction) const override
    {
        const vec3f light = unit_vector(direction);
        if (dot(light, frame.normal) <= 0.0f)
        {
            return 0.0f;
        }
        const vec3f half            = unit_vector(light + view);
        const float normal_dot_half = dot(half, frame.normal);
        const float view_dot_half   = dot(view, half);
        if (normal_dot_half <= 0.0f || view_dot_half <= 0.0f)
        {
            return 0.0f;
        }
        return anisotropic_ggx_distribution(dot(half, frame.tangent), dot(half, frame.bitangent),
                                            normal_dot_half, alpha_t, alpha_b) *
               normal_dot_half / (4.0f * view_dot_half);
    }

    vec3f generate() const override
    {
        const float angle   = 2.0f * PI * random_float();
        const float phi     = std::atan2(alpha_b * std::sin(angle), alpha_t * std::cos(angle));
        const float cos_phi = std::cos(phi), sin_phi = std::sin(phi);

        const float u = random_float();
        const float spread =
            cos_phi * cos_phi / (alpha_t * alpha_t) + sin_phi * sin_phi / (alpha_b * alpha_b);
        const float cos_theta = 1.0f / std::sqrt(1.0f + u / ((1.0f - u) * spread));
        const float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

        const vec3f half = sin_theta * cos_phi * frame.tangent +
                           sin_theta * sin_phi * frame.bitangent + cos_theta * frame.normal;
        return reflect(-view, half);
    }

private:
    anisotropic_frame frame;
    vec3f view;
    float alpha_t, alpha_b;
};

} // namespace rt
