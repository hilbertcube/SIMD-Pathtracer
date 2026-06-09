#pragma once

#include <cmath>
#include "rt/math/vector.h"
#include "rt/geometry/quad.h"
#include "rt/geometry/hittable_list.h"

namespace rt
{
class triangle : public hittable
{
public:
    triangle(const point3f& vertex_a, const point3f& vertex_b, const point3f& vertex_c,
             shared_ptr<material> surface_material)
        : vertex_a(vertex_a), vertex_b(vertex_b), vertex_c(vertex_c),
          surface_material(surface_material)
    {
        edge_ab      = vertex_b - vertex_a;
        edge_ac      = vertex_c - vertex_a;
        normal       = unit_vector(cross(edge_ab, edge_ac));
        plane_offset = dot(normal, vertex_a);
        set_bounding_box();
    }

    void set_bounding_box()
    {
        point3f min_point(std::fmin(std::fmin(vertex_a.x(), vertex_b.x()), vertex_c.x()),
                          std::fmin(std::fmin(vertex_a.y(), vertex_b.y()), vertex_c.y()),
                          std::fmin(std::fmin(vertex_a.z(), vertex_b.z()), vertex_c.z()));

        point3f max_point(std::fmax(std::fmax(vertex_a.x(), vertex_b.x()), vertex_c.x()),
                          std::fmax(std::fmax(vertex_a.y(), vertex_b.y()), vertex_c.y()),
                          std::fmax(std::fmax(vertex_a.z(), vertex_b.z()), vertex_c.z()));

        bounds = AABB(min_point, max_point);
    }

    AABB bounding_box() const override { return bounds; }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        float denominator = dot(normal, incoming.direction());
        if (std::fabs(denominator) < 1e-8)
        {
            return false;
        }

        float t = (plane_offset - dot(normal, incoming.origin())) / denominator;
        if (!t_range.contains(t))
        {
            return false;
        }

        point3f hit_point = incoming.at(t);
        // Inside-outside test using cross products
        vec3f cross_a = cross(vertex_b - vertex_a, hit_point - vertex_a);
        vec3f cross_b = cross(vertex_c - vertex_b, hit_point - vertex_b);
        vec3f cross_c = cross(vertex_a - vertex_c, hit_point - vertex_c);

        if (dot(normal, cross_a) < 0 || dot(normal, cross_b) < 0 || dot(normal, cross_c) < 0)
        {
            return false;
        }

        record.t                = t;
        record.point            = hit_point;
        record.surface_material = surface_material.get();
        record.set_face_normal(incoming, normal);
        return true;
    }

private:
    point3f vertex_a, vertex_b, vertex_c;
    vec3f edge_ab, edge_ac, normal;
    float plane_offset;
    shared_ptr<material> surface_material;
    AABB bounds;
};

inline shared_ptr<hittable_list> pyramid(const point3f& base_min, const point3f& base_max,
                                         const point3f& apex, shared_ptr<material> surface_material)
{
    auto sides = make_shared<hittable_list>();

    point3f corner_a(base_min.x(), base_min.y(), base_min.z());
    point3f corner_b(base_max.x(), base_min.y(), base_min.z());
    point3f corner_c(base_max.x(), base_min.y(), base_max.z());
    point3f corner_d(base_min.x(), base_min.y(), base_max.z());

    // Base (quad)
    sides->add(
        make_shared<quad>(corner_a, corner_b - corner_a, corner_d - corner_a, surface_material));

    // Four triangular sides
    sides->add(make_shared<triangle>(corner_a, corner_b, apex, surface_material));
    sides->add(make_shared<triangle>(corner_b, corner_c, apex, surface_material));
    sides->add(make_shared<triangle>(corner_c, corner_d, apex, surface_material));
    sides->add(make_shared<triangle>(corner_d, corner_a, apex, surface_material));

    return sides;
}

} // namespace rt
