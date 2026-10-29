#pragma once

#include "rt/materials/material.h"

namespace rt
{

class lambertian : public material
{
private:
    shared_ptr<texture> albedo_texture;

public:
    lambertian(const color& albedo) : albedo_texture(make_shared<solid_color>(albedo)) {}

    lambertian(shared_ptr<texture> albedo_texture) : albedo_texture(albedo_texture) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation   = albedo_texture->value(record.u, record.v, record.point);
        scattering.cosine        = cosine_pdf(record.normal);
        scattering.direction_pdf = &scattering.cosine;
        scattering.is_specular   = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        // cos(theta) without building the normalized vector
        float projection = dot(record.normal, scattered.direction());
        if (projection <= 0.0f)
        {
            return 0.0f;
        }
        return projection / (scattered.direction().length() * PI);
    }

    bool needs_uv() const override { return albedo_texture->needs_uv(); }
};

} // namespace rt
