#pragma once

#include "rt/materials/material.h"

namespace rt
{

/*
 * A scattering medium whose scattering favors one direction (mist, clouds, smoke, steam):
 * g > 0 scatters mostly forward, which makes it glow when lit from behind. Use it as the phase
 * function of a constant_medium.
 */
class mist : public material
{
public:
    mist(const color& albedo, float g) : albedo_texture(make_shared<solid_color>(albedo)), g(g) {}

    bool scatter(const ray& incoming, const hit_record& record,
                 scatter_record& scattering) const override
    {
        scattering.attenuation = albedo_texture->value(record.u, record.v, record.point);
        scattering.direction_pdf =
            scattering.emplace<henyey_greenstein_pdf>(unit_vector(incoming.direction()), g);
        scattering.is_specular = false;
        return true;
    }

    float scattering_pdf(const ray& incoming, const hit_record& record,
                         const ray& scattered) const override
    {
        return henyey_greenstein(
            dot(unit_vector(incoming.direction()), unit_vector(scattered.direction())), g);
    }

    bool needs_uv() const override { return false; }

private:
    shared_ptr<texture> albedo_texture;
    float g;
};

} // namespace rt
