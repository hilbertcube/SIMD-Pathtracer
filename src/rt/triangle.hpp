#pragma once

#include <cmath>
#include "rtm/vector.hpp"
#include "quad.hpp"
#include "hittable_list.hpp"

namespace rt
{
class triangle : public hittable {
public:
    triangle(const point3f& a, const point3f& b, const point3f& c, shared_ptr<material> mat)
        : a(a), b(b), c(c), mat(mat)
    {
        // Precompute edges and normal
        u = b - a;
        v = c - a;
        normal = unit_vector(cross(u, v));
        D = dot(normal, a);
        set_bounding_box();
    }

    void set_bounding_box() {
        point3f min_point(
            std::fmin(std::fmin(a.x(), b.x()), c.x()),
            std::fmin(std::fmin(a.y(), b.y()), c.y()),
            std::fmin(std::fmin(a.z(), b.z()), c.z())
        );

        point3f max_point(
            std::fmax(std::fmax(a.x(), b.x()), c.x()),
            std::fmax(std::fmax(a.y(), b.y()), c.y()),
            std::fmax(std::fmax(a.z(), b.z()), c.z())
        );

        bbox = AABB(min_point, max_point);
    }

    AABB bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        float denom = dot(normal, r.direction());
        if (std::fabs(denom) < 1e-8) return false;

        float t = (D - dot(normal, r.origin())) / denom;
        if (!ray_t.contains(t)) return false;

        point3f p = r.at(t);
        // Inside-outside test using cross products
        vec3f c0 = cross(b - a, p - a);
        vec3f c1 = cross(c - b, p - b);
        vec3f c2 = cross(a - c, p - c);

        if (dot(normal, c0) < 0 || dot(normal, c1) < 0 || dot(normal, c2) < 0)
            return false;

        rec.t = t;
        rec.p = p;
        rec.mat = mat;
        rec.set_face_normal(r, normal);
        return true;
    }

private:
    point3f a, b, c;
    vec3f u, v, normal;
    float D;
    shared_ptr<material> mat;
    AABB bbox;
};

inline shared_ptr<hittable_list> pyramid(
    const point3f& base_min, const point3f& base_max,
    const point3f& apex,
    shared_ptr<material> mat)
{
    auto sides = make_shared<hittable_list>();

    // Base corners
    point3f A(base_min.x(), base_min.y(), base_min.z());
    point3f B(base_max.x(), base_min.y(), base_min.z());
    point3f C(base_max.x(), base_min.y(), base_max.z());
    point3f D(base_min.x(), base_min.y(), base_max.z());

    // Base (quad)
    sides->add(make_shared<quad>(A, B - A, D - A, mat));

    // Four triangular sides
    sides->add(make_shared<triangle>(A, B, apex, mat));
    sides->add(make_shared<triangle>(B, C, apex, mat));
    sides->add(make_shared<triangle>(C, D, apex, mat));
    sides->add(make_shared<triangle>(D, A, apex, mat));

    return sides;
}

} // namespace rt



