#pragma once

#include <cmath>
#include "rt/geometry/hittable.h"
#include "rt/math/ONB.h"
#include "rt/math/constants.h"
#include "rt/math/random.h"
#include "rt/math/vector.h"

namespace rt
{

/// @brief Probability density function over directions.
struct pdf
{
    virtual ~pdf() = default;

    /// Density of sampling `direction` (need not be normalized).
    virtual float value(const vec3f& direction) const = 0;

    /// Draw a random direction distributed according to this pdf.
    virtual vec3f generate() const = 0;
};

/// @brief Uniform over the whole sphere of directions.
struct sphere_pdf : public pdf
{
    float value(const vec3f& direction) const override { return 1.0f / (4.0f * PI); }

    vec3f generate() const override { return random_unit_vector<float, 3>(); }
};

/// @brief Cosine-weighted hemisphere around a normal (ideal for Lambertian surfaces).
class cosine_pdf : public pdf
{
public:
    cosine_pdf() = default;

    explicit cosine_pdf(const vec3f& normal) : basis(normal) {}

    float value(const vec3f& direction) const override
    {
        // cos(theta) without building the normalized vector
        float projection = dot(direction, basis.normal());
        if (projection <= 0.0f)
        {
            return 0.0f;
        }
        return projection / (direction.length() * PI);
    }

    vec3f generate() const override { return basis.transform(random_cosine_direction<float>()); }

private:
    ONB basis;
};

// GGX (Trowbridge-Reitz) distribution of microfacet normals: the density of facets whose normal
// makes an angle with cosine `normal_dot_half` to the surface normal. alpha_squared is the square
// of the roughness parameter alpha.
inline float ggx_distribution(float normal_dot_half, float alpha_squared)
{
    const float denominator = normal_dot_half * normal_dot_half * (alpha_squared - 1.0f) + 1.0f;
    return alpha_squared / (PI * denominator * denominator);
}

// Smith masking for GGX: the share of microfacets visible from a direction whose angle to the
// normal has cosine `normal_dot_direction`
inline float ggx_masking(float normal_dot_direction, float alpha_squared)
{
    return 2.0f * normal_dot_direction /
           (normal_dot_direction +
            std::sqrt(alpha_squared +
                      (1.0f - alpha_squared) * normal_dot_direction * normal_dot_direction));
}

/*
 * The directions a rough conductor reflects into: pick a microfacet normal from the GGX
 * distribution, then mirror the view direction about it. `view` is the unit vector from the
 * surface toward the viewer.
 */
class ggx_pdf : public pdf
{
public:
    ggx_pdf(const vec3f& normal, const vec3f& view, float alpha)
        : basis(normal), view(view), alpha_squared(alpha * alpha)
    {
    }

    float value(const vec3f& direction) const override
    {
        const vec3f light = unit_vector(direction);
        if (dot(light, basis.normal()) <= 0.0f)
        {
            return 0.0f;
        }
        const vec3f half            = unit_vector(light + view);
        const float normal_dot_half = dot(half, basis.normal());
        const float view_dot_half   = dot(view, half);
        if (normal_dot_half <= 0.0f || view_dot_half <= 0.0f)
        {
            return 0.0f;
        }
        return ggx_distribution(normal_dot_half, alpha_squared) * normal_dot_half /
               (4.0f * view_dot_half);
    }

    vec3f generate() const override
    {
        const float u         = random_float();
        const float cos_theta = std::sqrt((1.0f - u) / (1.0f + (alpha_squared - 1.0f) * u));
        return reflect(-view, microfacet_normal(cos_theta));
    }

private:
    ONB basis;
    vec3f view;
    float alpha_squared;

    vec3f microfacet_normal(float cos_theta) const
    {
        const float sin_theta = std::sqrt(std::fmax(0.0f, 1.0f - cos_theta * cos_theta));
        const float phi       = 2.0f * PI * random_float();
        return basis.transform(
            vec3f(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta));
    }
};

// The Henyey-Greenstein phase function: how light scatters in a medium, as a density over
// directions. `cosine` is between the direction of travel and the direction scattered into; g in
// (-1, 1) is positive for forward scattering (clouds, mist), negative for back scattering, and 0
// for scattering equally in all directions.
inline float henyey_greenstein(float cosine, float g)
{
    const float denominator = 1.0f + g * g - 2.0f * g * cosine;
    return (1.0f - g * g) / (4.0f * PI * denominator * std::sqrt(denominator));
}

// Directions scattered by a medium with the Henyey-Greenstein phase function, for light that
// was traveling along the unit vector `forward`
class henyey_greenstein_pdf : public pdf
{
public:
    henyey_greenstein_pdf(const vec3f& forward, float g) : basis(forward), forward(forward), g(g) {}

    float value(const vec3f& direction) const override
    {
        return henyey_greenstein(dot(unit_vector(direction), forward), g);
    }

    vec3f generate() const override
    {
        float cosine;
        if (std::fabs(g) < 1e-3f)
        {
            cosine = 1.0f - 2.0f * random_float();
        }
        else
        {
            const float fraction = (1.0f - g * g) / (1.0f - g + 2.0f * g * random_float());
            cosine               = (1.0f + g * g - fraction * fraction) / (2.0f * g);
        }
        const float sine = std::sqrt(std::fmax(0.0f, 1.0f - cosine * cosine));
        const float phi  = 2.0f * PI * random_float();
        return basis.transform(vec3f(sine * std::cos(phi), sine * std::sin(phi), cosine));
    }

private:
    ONB basis;
    vec3f forward;
    float g;
};

} // namespace rt
