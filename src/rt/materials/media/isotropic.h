#pragma once

#include "rt/materials/material.h"

namespace rt
{

class isotropic : public material
{
public:
    isotropic(const color& albedo) : albedo_texture(make_shared<solid_color>(albedo)) {}

    isotropic(shared_ptr<texture> albedo_texture) : albedo_texture(albedo_texture) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.direction_pdf = &scattering.sphere;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return 1.0f / (4.0f * PI);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }

private:
    shared_ptr<texture> albedo_texture;
};

} // namespace rt
