#pragma once

#include "rt/materials/material.h"

namespace rt
{

class diffuse_light : public material
{
public:
    diffuse_light(shared_ptr<texture> emission_texture) : emission_texture(emission_texture) {}

    diffuse_light(const color& emission) : emission_texture(make_shared<solid_color>(emission)) {}

    // One-sided: only the front face (the side the surface normal points to) emits.
    color emitted(const ray& incoming, const hit_record& record, float u, float v,
                  const point3f& point) const override
    {
        if (!record.front_face)
        {
            return color(0, 0, 0);
        }
        return emission_texture->value(u, v, point);
    }

    bool needs_uv() const override { return emission_texture->needs_uv(); }

private:
    shared_ptr<texture> emission_texture;
};

} // namespace rt
