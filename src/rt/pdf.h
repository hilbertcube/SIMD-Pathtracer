#pragma once

#include <cmath>
#include "hittable.h"
#include "rtm/ONB.h"
#include "rtm/constants.h"
#include "rtm/random.h"
#include "rtm/vector.h"

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
        float cosine_theta = dot(unit_vector(direction), basis.normal());
        return std::fmax(0.0f, cosine_theta / PI);
    }

    vec3f generate() const override { return basis.transform(random_cosine_direction<float>()); }

private:
    ONB basis;
};

} // namespace rt
