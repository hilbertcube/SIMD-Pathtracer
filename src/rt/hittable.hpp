#pragma once

#include "rtm/ray.hpp"
#include "rtm/random.hpp"
#include "rtm/functions.hpp"
#include "rtm/vector.hpp"
#include "AABB.hpp"

namespace rt
{
class material;

class hit_record {
public:
    point3f p;
    vec3f normal;
    float t;
    float u, v;
    bool front_face;
    shared_ptr<material> mat;

    // Sets the hit record normal vector
    // NOTE: the @param outward_normal is assumed to have a unit length
    void set_face_normal(const ray& r, const vec3f& outward_normal)
    {
        front_face = (dot(r.direction(), outward_normal) < 0);
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// an abstract class of hittable objects
class hittable {
protected:
    hittable() {}
public:
    // default destructor
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual AABB bounding_box() const = 0;
};

class translate : public hittable {
public:
    translate(shared_ptr<hittable> object, const vec3f& offset)
        : object(object), offset(offset)
    {
        bbox = object->bounding_box() + offset;
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override 
    {
        // Move the ray backwards by the offset
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }

    AABB bounding_box() const override { return bbox; }

private:
    shared_ptr<hittable> object;
    vec3f offset;
    AABB bbox;
};

class rotate_y : public hittable {
public:
    rotate_y(shared_ptr<hittable> object, float angle) 
        : object(object) 
    {
        float radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        point3f min( INF,  INF,  INF);
        point3f max(-INF, -INF, -INF);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3f tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = std::fmin(min[c], tester[c]);
                        max[c] = std::fmax(max[c], tester[c]);
                    }
                }
            }
        }
        bbox = AABB(min, max);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Transform the ray from world space to object space.
        auto origin = point3f(
            (cos_theta * r.origin().x()) - (sin_theta * r.origin().z()),
            r.origin().y(),
            (sin_theta * r.origin().x()) + (cos_theta * r.origin().z())
        );

        auto direction = vec3f(
            (cos_theta * r.direction().x()) - (sin_theta * r.direction().z()),
            r.direction().y(),
            (sin_theta * r.direction().x()) + (cos_theta * r.direction().z())
        );

        ray rotated_r(origin, direction, r.time());

        // Determine whether an intersection exists in object space (and if so, where).
        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Transform the intersection from object space back to world space.
        rec.p = point3f(
            (cos_theta * rec.p.x()) + (sin_theta * rec.p.z()),
            rec.p.y(),
            (-sin_theta * rec.p.x()) + (cos_theta * rec.p.z())
        );

        rec.normal = vec3f(
            (cos_theta * rec.normal.x()) + (sin_theta * rec.normal.z()),
            rec.normal.y(),
            (-sin_theta * rec.normal.x()) + (cos_theta * rec.normal.z())
        );

        return true;
    }

    AABB bounding_box() const override { return bbox; }
private:
    shared_ptr<hittable> object;
    float sin_theta;
    float cos_theta;
    AABB bbox;
};


} // namespace rt


