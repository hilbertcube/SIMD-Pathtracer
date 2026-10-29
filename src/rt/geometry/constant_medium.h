#pragma once

#include "rt/geometry/hittable.h"
#include "rt/materials/media/isotropic.h"
#include "rt/materials/textures/texture.h"

namespace rt
{
class constant_medium : public hittable
{
public:
    constant_medium(shared_ptr<hittable> boundary, float density,
                    shared_ptr<texture> albedo_texture)
        : boundary(boundary), negative_inverse_density(-1 / density),
          phase_function(make_shared<isotropic>(albedo_texture))
    {
    }

    // Any material can be the phase function, for example anisotropic or glowing_medium
    constant_medium(shared_ptr<hittable> boundary, float density,
                    shared_ptr<material> phase_function)
        : boundary(boundary), negative_inverse_density(-1 / density), phase_function(phase_function)
    {
    }

    constant_medium(shared_ptr<hittable> boundary, float density, const color& albedo)
        : boundary(boundary), negative_inverse_density(-1 / density),
          phase_function(make_shared<isotropic>(albedo))
    {
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        hit_record entry_record, exit_record;

        if (!boundary->hit(incoming, interval::universe, entry_record))
        {
            return false;
        }

        if (!boundary->hit(incoming, interval(entry_record.t + 0.0001, INF), exit_record))
        {
            return false;
        }

        if (entry_record.t < t_range.min)
        {
            entry_record.t = t_range.min;
        }
        if (exit_record.t > t_range.max)
        {
            exit_record.t = t_range.max;
        }

        if (entry_record.t >= exit_record.t)
        {
            return false;
        }

        if (entry_record.t < 0)
        {
            entry_record.t = 0;
        }

        auto ray_length               = incoming.direction().length();
        auto distance_inside_boundary = (exit_record.t - entry_record.t) * ray_length;
        auto hit_distance             = negative_inverse_density * std::log(random_float());

        if (hit_distance > distance_inside_boundary)
        {
            return false;
        }

        record.t     = entry_record.t + hit_distance / ray_length;
        record.point = incoming.at(record.t);

        record.normal           = vec3f(1, 0, 0); // arbitrary
        record.front_face       = true;           // also arbitrary
        record.surface_material = phase_function.get();

        return true;
    }

    AABB bounding_box() const override { return boundary->bounding_box(); }

private:
    shared_ptr<hittable> boundary;
    float negative_inverse_density;
    shared_ptr<material> phase_function;
};
} // namespace rt
