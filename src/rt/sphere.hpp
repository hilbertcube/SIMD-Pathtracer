#pragma once

#include <cmath>
#include "hittable.hpp"
#include "material.hpp"

// #include "rtm/vec3.hpp"
#include "rtm/random.hpp"
#include "rtm/vector.hpp"
#include "rtm/ray.hpp"
#include "rtm/constants.hpp"

namespace rt {

class sphere: public hittable {
private:
    ray center;
    float radius;
    shared_ptr<material> mat;
    AABB bbox;
public:
    // Stationary Sphere
    sphere(const point3f& static_center, float radius, shared_ptr<material> mat)
      : center(static_center, vec3f(0,0,0)), radius(std::fmax(0,radius)), mat(mat) 
    {
        auto rvec = vec3f(radius, radius, radius);
        bbox = AABB(static_center - rvec, static_center + rvec);
    }

    // Moving Sphere
    sphere(const point3f& center1, const point3f& center2, float radius,
           shared_ptr<material> mat)
      : center(center1, center2 - center1), radius(std::fmax(0,radius)), mat(mat) 
    {
        auto rvec = vec3f(radius, radius, radius);
        AABB box1(center.at(0) - rvec, center.at(0) + rvec);
        AABB box2(center.at(1) - rvec, center.at(1) + rvec);
        bbox = AABB(box1, box2);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        point3f current_center = center.at(r.time());
        vec3f oc = current_center - r.origin();
        auto a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = h * h - a * c;
        if(discriminant < 0) return false;

        auto sqrtd = std::sqrt(discriminant);
        
        // Find the nearest root that lies in the acceptable range
        auto root = (h - sqrtd) / a;
        if(root <= ray_t.min || ray_t.max <= root) {
            root = (h + sqrtd) / a;
            if(root <= ray_t.min || ray_t.max <= root)
                return false;
        }
        rec.t = root;
        rec.p = r.at(rec.t);
        vec3f outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;

        return true;
    }

    // p: a given point on the sphere of radius one, centered at the origin.
    // u: returned value [0,1] of angle around the Y axis from X=-1.
    // v: returned value [0,1] of angle from Y=-1 to Y=+1.
    // Ex:
    //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
    //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
    //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>
    static void get_sphere_uv(const point3f& p, float& u, float& v) {
        float theta = std::acos(-p.y());
        float phi = std::atan2(-p.z(), p.x()) + PI;

        u = phi / (2 * PI);
        v = theta / PI;
    }

    AABB bounding_box() const override
    {
        return bbox;
    }
};

} // namespace rt