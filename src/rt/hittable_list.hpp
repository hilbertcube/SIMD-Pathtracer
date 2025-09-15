#pragma once

#include <memory>
#include <vector>

#include "hittable.hpp"
#include "rtm/random.hpp"

namespace rt{

// a list of hittable objects 
class hittable_list : public hittable
{
public:
    // an array of pointers to hittable objects
    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object)
    {
        objects.push_back(object);
        bbox = AABB(bbox, object->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closes_so_far = ray_t.max;

        for(const auto& object: objects) {
            if(object->hit(r, interval(ray_t.min, closes_so_far), temp_rec)){
                hit_anything = true;
                closes_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    AABB bounding_box() const override { return bbox; }

private:
    AABB bbox;
};

}
