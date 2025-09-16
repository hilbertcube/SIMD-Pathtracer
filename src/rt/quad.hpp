#pragma once

// #include "rtm/vec3.hpp"
#include <cmath>
#include "rtm/vector.hpp"
#include "hittable_list.hpp"

namespace rt
{
class quad : public hittable {
public:
    // Q: starting corner
    // u: vector represent the first side
    // v: vector representing the second side
    // basically: three corners
    // mat: material
    quad(const point3f& Q, const vec3f& u, const vec3f& v, shared_ptr<material> mat)
      : Q(Q), u(u), v(v), mat(mat)
    {
        auto n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, Q);
        w = n / dot(n, n);
        set_bounding_box();
    }

    virtual void set_bounding_box()
    {
        // Compute the bounding box of all four vertices.
        auto bbox_diagonal1 = AABB(Q, Q + u + v);
        auto bbox_diagonal2 = AABB(Q + u, Q + v);
        bbox = AABB(bbox_diagonal1, bbox_diagonal2);
    }

    AABB bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override 
    {
        float denominator = dot(normal, r.direction());

        // No hit if the ray is parallel to the plane.
        if (std::fabs(denominator) < 1e-8) return false;

        // Return false if the hit point parameter t is outside the ray interval.
        float t = (D - dot(normal, r.origin())) / denominator;
        if (!ray_t.contains(t)) return false;

        point3f intersection = r.at(t);
        vec3f planar_hitpt_vector = intersection - Q;
        auto alpha = dot(w, cross(planar_hitpt_vector, v));
        auto beta = dot(w, cross(u, planar_hitpt_vector));

        if (!is_interior(alpha, beta, rec))
            return false;

        // Ray hits the 2D shape; set the rest of the hit record and return true.
        rec.t = t;
        rec.p = intersection;
        rec.mat = mat;
        rec.set_face_normal(r, normal);

        return true;
    }

    virtual bool is_interior(float a, float b, hit_record& rec) const 
    {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        if (!unit_interval.contains(a) || !unit_interval.contains(b))
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }

private:
    point3f Q;
    vec3f u, v, w;
    shared_ptr<material> mat;
    AABB bbox;
    vec3f normal;
    float D;
};

inline shared_ptr<hittable_list> box(const point3f& a, const point3f& b, shared_ptr<material> mat)
{
    // Returns the 3D box (six sides) that contains the two opposite vertices a & b.

    auto sides = make_shared<hittable_list>();

    // Construct the two opposite vertices with the minimum and maximum coordinates.
    auto min = point3f(std::fmin(a.x(),b.x()), std::fmin(a.y(),b.y()), std::fmin(a.z(),b.z()));
    auto max = point3f(std::fmax(a.x(),b.x()), std::fmax(a.y(),b.y()), std::fmax(a.z(),b.z()));

    auto dx = vec3f(max.x() - min.x(), 0, 0);
    auto dy = vec3f(0, max.y() - min.y(), 0);
    auto dz = vec3f(0, 0, max.z() - min.z());

    sides->add(make_shared<quad>(point3f(min.x(), min.y(), max.z()),  dx,  dy, mat)); // front
    sides->add(make_shared<quad>(point3f(max.x(), min.y(), max.z()), -dz,  dy, mat)); // right
    sides->add(make_shared<quad>(point3f(max.x(), min.y(), min.z()), -dx,  dy, mat)); // back
    sides->add(make_shared<quad>(point3f(min.x(), min.y(), min.z()),  dz,  dy, mat)); // left
    sides->add(make_shared<quad>(point3f(min.x(), max.y(), max.z()),  dx, -dz, mat)); // top
    sides->add(make_shared<quad>(point3f(min.x(), min.y(), min.z()),  dx,  dz, mat)); // bottom

    return sides;
}

} // namespace rt



