#pragma once

#include "rtm/ray.h"
#include "rtm/random.h"
#include "rtm/functions.h"
#include "rtm/vector.h"
#include "AABB.h"

namespace rt
{
class material;

class hit_record
{
public:
    point3f point;
    vec3f normal;
    float t;
    float u, v;
    bool front_face;
    const material* surface_material = nullptr; // Non-owning: objects keep their material alive

    // Sets the hit record normal vector
    // NOTE: the @param outward_normal is assumed to have a unit length
    void set_face_normal(const ray& incoming, const vec3f& outward_normal)
    {
        front_face = (dot(incoming.direction(), outward_normal) < 0);
        normal     = front_face ? outward_normal : -outward_normal;
    }
};

// an abstract class of hittable objects
class hittable
{
protected:
    hittable() {}

public:
    // default destructor
    virtual ~hittable() = default;

    virtual bool hit(const ray& incoming, interval t_range, hit_record& record) const = 0;

    virtual AABB bounding_box() const = 0;

    /// Density (over solid angle) of picking `direction` from `origin` via random().
    /// Only needed for objects used as lights in importance sampling.
    virtual float pdf_value(const point3f& origin, const vec3f& direction) const { return 0.0f; }

    /// Random direction from `origin` toward this object.
    virtual vec3f random(const point3f& origin) const { return vec3f(1, 0, 0); }
};

class translate : public hittable
{
public:
    translate(shared_ptr<hittable> object, const vec3f& offset) : object(object), offset(offset)
    {
        bounds = object->bounding_box() + offset;
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        // Move the ray backwards by the offset
        ray offset_ray(incoming.origin() - offset, incoming.direction(), incoming.time());

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_ray, t_range, record))
        {
            return false;
        }

        // Move the intersection point forwards by the offset
        record.point += offset;

        return true;
    }

    AABB bounding_box() const override { return bounds; }

private:
    shared_ptr<hittable> object;
    vec3f offset;
    AABB bounds;
};

class rotate_y : public hittable
{
public:
    rotate_y(shared_ptr<hittable> object, float angle_degrees) : object(object)
    {
        float radians = degrees_to_radians(angle_degrees);
        sin_theta     = std::sin(radians);
        cos_theta     = std::cos(radians);
        bounds        = object->bounding_box();

        point3f min(INF, INF, INF);
        point3f max(-INF, -INF, -INF);

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    auto x = i * bounds.x.max + (1 - i) * bounds.x.min;
                    auto y = j * bounds.y.max + (1 - j) * bounds.y.min;
                    auto z = k * bounds.z.max + (1 - k) * bounds.z.min;

                    auto rotated_x = cos_theta * x + sin_theta * z;
                    auto rotated_z = -sin_theta * x + cos_theta * z;

                    vec3f tester(rotated_x, y, rotated_z);

                    for (int axis = 0; axis < 3; axis++)
                    {
                        min[axis] = std::fmin(min[axis], tester[axis]);
                        max[axis] = std::fmax(max[axis], tester[axis]);
                    }
                }
            }
        }
        bounds = AABB(min, max);
    }

    // avoid annoying type cast warning
    template <typename T>
    rotate_y(std::shared_ptr<hittable> object, T angle_degrees)
        : rotate_y(object, static_cast<float>(angle_degrees))
    {
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        // Transform the ray from world space to object space.
        auto origin =
            point3f((cos_theta * incoming.origin().x()) - (sin_theta * incoming.origin().z()),
                    incoming.origin().y(),
                    (sin_theta * incoming.origin().x()) + (cos_theta * incoming.origin().z()));

        auto direction =
            vec3f((cos_theta * incoming.direction().x()) - (sin_theta * incoming.direction().z()),
                  incoming.direction().y(),
                  (sin_theta * incoming.direction().x()) + (cos_theta * incoming.direction().z()));

        ray rotated_ray(origin, direction, incoming.time());

        // Determine whether an intersection exists in object space (and if so, where).
        if (!object->hit(rotated_ray, t_range, record))
        {
            return false;
        }

        // Transform the intersection from object space back to world space.
        record.point = point3f((cos_theta * record.point.x()) + (sin_theta * record.point.z()),
                               record.point.y(),
                               (-sin_theta * record.point.x()) + (cos_theta * record.point.z()));

        record.normal = vec3f((cos_theta * record.normal.x()) + (sin_theta * record.normal.z()),
                              record.normal.y(),
                              (-sin_theta * record.normal.x()) + (cos_theta * record.normal.z()));

        return true;
    }

    AABB bounding_box() const override { return bounds; }

private:
    shared_ptr<hittable> object;
    float sin_theta;
    float cos_theta;
    AABB bounds;
};

} // namespace rt
