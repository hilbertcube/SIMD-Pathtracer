#pragma once

#include <cmath>
#include "hittable.h"
#include "material.h"

// #include "rtm/vec3.h"
#include "rtm/random.h"
#include "rtm/vector.h"
#include "rtm/ray.h"
#include "rtm/constants.h"
#include "rtm/ONB.h"

namespace rt
{

class sphere : public hittable
{
private:
    ray center;
    float radius;
    shared_ptr<material> mat;
    AABB bbox;

public:
    // Stationary Sphere
    sphere(const point3f& static_center, float radius, shared_ptr<material> mat)
        : center(static_center, vec3f(0, 0, 0)), radius(std::fmax(0, radius)), mat(mat)
    {
        auto rvec = vec3f(radius, radius, radius);
        bbox      = AABB(static_center - rvec, static_center + rvec);
    }

    // Moving Sphere
    sphere(const point3f& center1, const point3f& center2, float radius, shared_ptr<material> mat)
        : center(center1, center2 - center1), radius(std::fmax(0, radius)), mat(mat)
    {
        auto rvec = vec3f(radius, radius, radius);
        AABB box1(center.at(0) - rvec, center.at(0) + rvec);
        AABB box2(center.at(1) - rvec, center.at(1) + rvec);
        bbox = AABB(box1, box2);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        point3f current_center = center.at(r.time());
        vec3f oc               = current_center - r.origin();
        auto a                 = r.direction().length_squared();
        auto h                 = dot(r.direction(), oc);
        auto c                 = oc.length_squared() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range
        auto root = (h - sqrtd) / a;
        if (root <= ray_t.min || ray_t.max <= root)
        {
            root = (h + sqrtd) / a;
            if (root <= ray_t.min || ray_t.max <= root)
                return false;
        }
        rec.t                = root;
        rec.p                = r.at(rec.t);
        vec3f outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat.get();

        return true;
    }

    /*
     * p: a given point on the sphere of radius one, centered at the origin.
     * u: returned value [0,1] of angle around the Y axis from X=-1.
     * v: returned value [0,1] of angle from Y=-1 to Y=+1.
     * Ex:
     *     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
     *     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
     *     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>
     */
    static void get_sphere_uv(const point3f& p, float& u, float& v)
    {
        float theta = std::acos(-p.y());
        float phi   = std::atan2(-p.z(), p.x()) + PI;

        u = phi / (2 * PI);
        v = theta / PI;
    }

    // Light sampling. Only valid for stationary spheres (uses the center at time 0).
    float pdf_value(const point3f& origin, const vec3f& direction) const override
    {
        hit_record rec;
        if (!this->hit(ray(origin, direction), interval(0.001f, INF), rec))
            return 0.0f;

        float dist_squared = (center.at(0) - origin).length_squared();
        if (dist_squared <= radius * radius) // origin inside the sphere
            return 1.0f / (4.0f * PI);

        float cos_theta_max = std::sqrt(1.0f - radius * radius / dist_squared);
        float solid_angle   = 2.0f * PI * (1.0f - cos_theta_max);
        return 1.0f / solid_angle;
    }

    vec3f random(const point3f& origin) const override
    {
        vec3f direction        = center.at(0) - origin;
        float distance_squared = direction.length_squared();
        if (distance_squared <= radius * radius) // origin inside the sphere
            return random_unit_vector<float, 3>();

        ONB uvw(direction);
        return uvw.transform(random_to_sphere(radius, distance_squared));
    }

    AABB bounding_box() const override { return bbox; }

private:
    // Random direction inside the cone (around +z) that a sphere of `radius`
    // at `distance_squared` subtends.
    static vec3f random_to_sphere(float radius, float distance_squared)
    {
        float r1 = random_float();
        float r2 = random_float();
        float z  = 1.0f + r2 * (std::sqrt(1.0f - radius * radius / distance_squared) - 1.0f);

        float phi = 2.0f * PI * r1;
        float x   = std::cos(phi) * std::sqrt(1.0f - z * z);
        float y   = std::sin(phi) * std::sqrt(1.0f - z * z);

        return vec3f(x, y, z);
    }
};

} // namespace rt
