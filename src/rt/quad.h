#pragma once

#include <cmath>
#include "rtm/vector.h"
#include "hittable_list.h"

namespace rt
{
class quad : public hittable
{
public:
    /*
     * corner:           one corner of the parallelogram
     * edge_u:           vector along the first side, from `corner`
     * edge_v:           vector along the second side, from `corner`
     * surface_material: material of the quad
     */
    quad(const point3f& corner, const vec3f& edge_u, const vec3f& edge_v,
         shared_ptr<material> surface_material)
        : corner(corner), edge_u(edge_u), edge_v(edge_v), surface_material(surface_material)
    {
        auto plane_normal = cross(edge_u, edge_v);
        normal            = unit_vector(plane_normal);
        plane_offset      = dot(normal, corner);
        area              = plane_normal.length();

        /*
         * The hit point's plane coordinates are alpha = w . (p x v) and beta = w . (u x p),
         * where p runs from the corner to the hit point and w = n / (n . n). By the scalar
         * triple product these equal p . (v x w) and p . (w x u), so the two cross products
         * are computed here once instead of on every hit.
         */
        vec3f plane_basis = plane_normal / dot(plane_normal, plane_normal);
        alpha_axis        = cross(edge_v, plane_basis);
        beta_axis         = cross(plane_basis, edge_u);
        set_bounding_box();
    }

    void set_bounding_box()
    {
        // Compute the bounding box of all four vertices.
        auto diagonal_bounds1 = AABB(corner, corner + edge_u + edge_v);
        auto diagonal_bounds2 = AABB(corner + edge_u, corner + edge_v);
        bounds                = AABB(diagonal_bounds1, diagonal_bounds2);
    }

    AABB bounding_box() const override { return bounds; }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        float denominator = dot(normal, incoming.direction());

        // No hit if the ray is parallel to the plane.
        if (std::fabs(denominator) < 1e-8f)
        {
            return false;
        }

        // Return false if the hit point parameter t is outside the ray interval.
        float t = (plane_offset - dot(normal, incoming.origin())) / denominator;
        if (!t_range.contains(t))
        {
            return false;
        }

        point3f intersection = incoming.at(t);
        vec3f corner_to_hit  = intersection - corner;
        float alpha          = dot(corner_to_hit, alpha_axis);
        float beta           = dot(corner_to_hit, beta_axis);

        if (!is_interior(alpha, beta, record))
        {
            return false;
        }

        // Ray hits the 2D shape; set the rest of the hit record and return true.
        record.t                = t;
        record.point            = intersection;
        record.surface_material = surface_material.get();
        record.set_face_normal(incoming, normal);

        return true;
    }

    bool is_interior(float alpha, float beta, hit_record& record) const
    {
        interval unit_interval = interval(0, 1);
        // Given the hit point in plane coordinates, return false if it is outside the
        // primitive, otherwise set the hit record UV coordinates and return true.

        if (!unit_interval.contains(alpha) || !unit_interval.contains(beta))
        {
            return false;
        }

        record.u = alpha;
        record.v = beta;
        return true;
    }

    // Light sampling: pick a uniform point on the quad, convert area density to solid angle.
    float pdf_value(const point3f& origin, const vec3f& direction) const override
    {
        hit_record record;
        if (!this->hit(ray(origin, direction), interval(0.001f, INF), record))
        {
            return 0.0f;
        }

        float distance_squared = record.t * record.t * direction.length_squared();
        float cosine           = std::fabs(dot(direction, record.normal) / direction.length());
        if (cosine <= 0.0f)
        {
            return 0.0f;
        }

        return distance_squared / (cosine * area);
    }

    vec3f random(const point3f& origin) const override
    {
        point3f point_on_quad = corner + (random_float() * edge_u) + (random_float() * edge_v);
        return point_on_quad - origin;
    }

private:
    point3f corner;
    vec3f edge_u, edge_v;
    vec3f alpha_axis, beta_axis; // project a corner-relative point onto plane coordinates
    shared_ptr<material> surface_material;
    AABB bounds;
    vec3f normal;
    float plane_offset;
    float area;
};

inline shared_ptr<hittable_list> box(const point3f& corner_a, const point3f& corner_b,
                                     shared_ptr<material> surface_material)
{
    // Returns the 3D box (six sides) that contains the two opposite vertices corner_a & corner_b.

    auto sides = make_shared<hittable_list>();

    // Construct the two opposite vertices with the minimum and maximum coordinates.
    auto min = point3f(std::fmin(corner_a.x(), corner_b.x()), std::fmin(corner_a.y(), corner_b.y()),
                       std::fmin(corner_a.z(), corner_b.z()));
    auto max = point3f(std::fmax(corner_a.x(), corner_b.x()), std::fmax(corner_a.y(), corner_b.y()),
                       std::fmax(corner_a.z(), corner_b.z()));

    auto size_x = vec3f(max.x() - min.x(), 0, 0);
    auto size_y = vec3f(0, max.y() - min.y(), 0);
    auto size_z = vec3f(0, 0, max.z() - min.z());

    sides->add(make_shared<quad>(point3f(min.x(), min.y(), max.z()), size_x, size_y,
                                 surface_material)); // front
    sides->add(make_shared<quad>(point3f(max.x(), min.y(), max.z()), -size_z, size_y,
                                 surface_material)); // right
    sides->add(make_shared<quad>(point3f(max.x(), min.y(), min.z()), -size_x, size_y,
                                 surface_material)); // back
    sides->add(make_shared<quad>(point3f(min.x(), min.y(), min.z()), size_z, size_y,
                                 surface_material)); // left
    sides->add(make_shared<quad>(point3f(min.x(), max.y(), max.z()), size_x, -size_z,
                                 surface_material)); // top
    sides->add(make_shared<quad>(point3f(min.x(), min.y(), min.z()), size_x, size_z,
                                 surface_material)); // bottom

    return sides;
}

} // namespace rt
