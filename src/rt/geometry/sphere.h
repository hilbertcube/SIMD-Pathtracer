#pragma once

#include <cmath>
#include "rt/geometry/hittable.h"
#include "rt/materials/material.h"

#include "rt/math/random.h"
#include "rt/math/vector.h"
#include "rt/math/ray.h"
#include "rt/math/constants.h"
#include "rt/math/ONB.h"

namespace rt
{

class sphere : public hittable
{
private:
    ray center;
    float radius;
    shared_ptr<material> surface_material;
    bool needs_uv; // only image textures read (u, v); acos and atan2 are costly
    AABB bounds;

public:
    // Stationary Sphere
    sphere(const point3f& static_center, float radius, shared_ptr<material> surface_material)
        : center(static_center, vec3f(0, 0, 0)), radius(std::fmax(0, radius)),
          surface_material(surface_material),
          needs_uv(surface_material && surface_material->needs_uv())
    {
        auto radius_vector = vec3f(radius, radius, radius);
        bounds             = AABB(static_center - radius_vector, static_center + radius_vector);
    }

    // Moving Sphere
    sphere(const point3f& center1, const point3f& center2, float radius,
           shared_ptr<material> surface_material)
        : center(center1, center2 - center1), radius(std::fmax(0, radius)),
          surface_material(surface_material),
          needs_uv(surface_material && surface_material->needs_uv())
    {
        auto radius_vector = vec3f(radius, radius, radius);
        AABB start_bounds(center.at(0) - radius_vector, center.at(0) + radius_vector);
        AABB end_bounds(center.at(1) - radius_vector, center.at(1) + radius_vector);
        bounds = AABB(start_bounds, end_bounds);
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        point3f current_center = center.at(incoming.time());
        vec3f origin_to_center = current_center - incoming.origin();
        auto direction_squared = incoming.direction().length_squared();
        auto projection        = dot(incoming.direction(), origin_to_center);
        auto center_term       = origin_to_center.length_squared() - radius * radius;

        auto discriminant = projection * projection - direction_squared * center_term;
        if (discriminant < 0)
        {
            return false;
        }

        auto sqrt_discriminant = std::sqrt(discriminant);

        auto root = (projection - sqrt_discriminant) / direction_squared;
        if (root <= t_range.min || t_range.max <= root)
        {
            root = (projection + sqrt_discriminant) / direction_squared;
            if (root <= t_range.min || t_range.max <= root)
            {
                return false;
            }
        }
        record.t             = root;
        record.point         = incoming.at(record.t);
        vec3f outward_normal = (record.point - current_center) / radius;
        record.set_face_normal(incoming, outward_normal);
        if (needs_uv)
        {
            get_sphere_uv(outward_normal, record.u, record.v);
        }
        else
        {
            record.u = record.v = 0.0f;
        }
        record.surface_material = surface_material.get();

        return true;
    }

    // Texture coordinates of a point on the unit sphere centered at the origin: u is the angle
    // around the Y axis from X = -1 and v the angle from Y = -1 to Y = +1, both in [0, 1]. For
    // example <1 0 0> gives <0.50 0.50> and <0 1 0> gives <0.50 1.00>.
    static void get_sphere_uv(const point3f& unit_point, float& u, float& v)
    {
        float theta = std::acos(-unit_point.y());
        float phi   = std::atan2(-unit_point.z(), unit_point.x()) + PI;

        u = phi / (2 * PI);
        v = theta / PI;
    }

    // Light sampling. Only valid for stationary spheres (uses the center at time 0).
    float pdf_value(const point3f& origin, const vec3f& direction) const override
    {
        hit_record record;
        if (!this->hit(ray(origin, direction), interval(0.001f, INF), record))
        {
            return 0.0f;
        }

        float distance_squared = (center.at(0) - origin).length_squared();
        if (distance_squared <= radius * radius) // origin inside the sphere
        {
            return 1.0f / (4.0f * PI);
        }

        float solid_angle = 2.0f * PI * cone_height(radius, distance_squared);
        return 1.0f / solid_angle;
    }

    vec3f random(const point3f& origin) const override
    {
        vec3f direction        = center.at(0) - origin;
        float distance_squared = direction.length_squared();
        if (distance_squared <= radius * radius) // origin inside the sphere
        {
            return random_unit_vector<float, 3>();
        }

        ONB basis(direction);
        return basis.transform(random_to_sphere(radius, distance_squared));
    }

    AABB bounding_box() const override { return bounds; }

private:
    /*
     * 1 - cos(theta_max) of the cone the sphere subtends, as x / (1 + sqrt(1 - x)) with
     * x = r^2 / d^2. The direct 1 - sqrt(1 - x) loses its precision in float for small, distant
     * spheres.
     */
    static float cone_height(float radius, float distance_squared)
    {
        const float x = radius * radius / distance_squared;
        return x / (1.0f + std::sqrt(1.0f - x));
    }

    // Random direction inside the cone (around +z) that a sphere of `radius`
    // at `distance_squared` subtends.
    static vec3f random_to_sphere(float radius, float distance_squared)
    {
        float first_sample  = random_float();
        float second_sample = random_float();
        float w             = second_sample * cone_height(radius, distance_squared); // 1 - z
        float z             = 1.0f - w;
        float sin_theta     = std::sqrt(w * (2.0f - w)); // sqrt(1 - z^2), stably

        float phi = 2.0f * PI * first_sample;
        float x   = std::cos(phi) * sin_theta;
        float y   = std::sin(phi) * sin_theta;

        return vec3f(x, y, z);
    }
};

} // namespace rt
