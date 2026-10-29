#pragma once

#include "rt/materials/material.h"
#include "rt/materials/textures/perlin.h"

namespace rt
{

/*
 * Bump mapping: tilts the surface normal as if the surface had small procedural dents and
 * bumps, then shades with another material. Hammered metal, orange peel, rough stone, with no
 * extra geometry.
 */
class bumped : public material
{
public:
    // feature_scale is the number of bumps per unit length; height the bumps' height in units
    bumped(shared_ptr<material> base, float feature_scale, float height)
        : base(base), feature_scale(feature_scale), height(height)
    {
    }

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        return base->scatter(incoming, perturbed(incoming, record), scattering);
    }

    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        return base->emitted(incoming, perturbed(incoming, record), u, v, point);
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return base->scattering_pdf(incoming, perturbed(incoming, record), scattered);
    }

    bool needs_uv() const override { return base->needs_uv(); }

private:
    shared_ptr<material> base;
    perlin noise;
    float feature_scale;
    float height;

    // The hit with its normal tilted away from the uphill direction of the bump field
    hit_record perturbed(const ray& incoming, const hit_record& record) const
    {
        const point3f p      = feature_scale * record.point;
        const float step     = 0.05f;
        const vec3f gradient = (height * feature_scale / (2.0f * step)) *
                               vec3f(noise.noise_smooth(p + vec3f(step, 0, 0)) -
                                         noise.noise_smooth(p - vec3f(step, 0, 0)),
                                     noise.noise_smooth(p + vec3f(0, step, 0)) -
                                         noise.noise_smooth(p - vec3f(0, step, 0)),
                                     noise.noise_smooth(p + vec3f(0, 0, step)) -
                                         noise.noise_smooth(p - vec3f(0, 0, step)));

        hit_record shaded = record;
        const vec3f tilted =
            unit_vector(record.normal - (gradient - dot(gradient, record.normal) * record.normal));
        if (dot(tilted, incoming.direction()) < 0.0f) // keep facing the ray
        {
            shaded.normal = tilted;
        }
        return shaded;
    }
};

} // namespace rt
