#pragma once

#include <cmath>
#include "hittable.h"
#include "rtm/ONB.h"
#include "rtm/constants.h"
#include "rtm/random.h"
#include "rtm/vector.h"

namespace rt {

/// @brief Probability density function over directions.
struct pdf {
    virtual ~pdf() = default;

    /// Density of sampling `direction` (need not be normalized).
    virtual float value(const vec3f& direction) const = 0;

    /// Draw a random direction distributed according to this pdf.
    virtual vec3f generate() const = 0;
};

/// @brief Uniform over the whole sphere of directions.
struct sphere_pdf : public pdf {
    float value(const vec3f& direction) const override
    {
        return 1.0f / (4.0f * PI);
    }

    vec3f generate() const override
    {
        return random_unit_vector<float, 3>();
    }
};

/// @brief Cosine-weighted hemisphere around a normal (ideal for Lambertian surfaces).
class cosine_pdf : public pdf {
public:
    cosine_pdf() = default;
    explicit cosine_pdf(const vec3f& w) : uvw(w) {}

    float value(const vec3f& direction) const override
    {
        float cosine_theta = dot(unit_vector(direction), uvw.w());
        return std::fmax(0.0f, cosine_theta / PI);
    }

    vec3f generate() const override
    {
        return uvw.transform(random_cosine_direction<float>());
    }

private:
    ONB uvw;
};

/// @brief Directions from `origin` toward a hittable (e.g. a light), using its pdf_value()/random().
class hittable_pdf : public pdf {
public:
    hittable_pdf(const hittable& objects, const point3f& origin)
        : objects(objects), origin(origin)
    {}

    float value(const vec3f& direction) const override
    {
        return objects.pdf_value(origin, direction);
    }

    vec3f generate() const override
    {
        return objects.random(origin);
    }

private:
    const hittable& objects;
    point3f origin;
};

/// @brief 50/50 mixture of two pdfs. Holds references: both must outlive it.
class mixture_pdf : public pdf {
public:
    mixture_pdf(const pdf& p0, const pdf& p1) : p0(p0), p1(p1) {}

    float value(const vec3f& direction) const override
    {
        return 0.5f * p0.value(direction) + 0.5f * p1.value(direction);
    }

    vec3f generate() const override
    {
        return random_float() < 0.5f ? p0.generate() : p1.generate();
    }

private:
    const pdf& p0;
    const pdf& p1;
};

} // namespace rt
