#pragma once

#include <string>

#include "rt/acceleration/bvh_node.h"
#include "rt/geometry/mesh.h"
#include "rt/materials/material.h"

namespace scenes
{

/*
 * Load a model, stand it `height` tall with the middle of its base at `base_center`, and
 * return it in its own BVH. `tilt_x` turns about x first (for models stored Z-up), then
 * `turn_y` turns it about the vertical axis to face the camera.
 */
inline rt::shared_ptr<rt::hittable> load_model(const std::string& filename,
                                               rt::shared_ptr<rt::material> material, float height,
                                               const rt::point3f& base_center, float turn_y = 0.0f,
                                               float tilt_x = 0.0f)
{
    auto mesh = rt::load_obj(filename, material);
    if (tilt_x != 0.0f)
    {
        rt::rotate_mesh(*mesh, 0, tilt_x);
    }
    if (turn_y != 0.0f)
    {
        rt::rotate_mesh(*mesh, 1, turn_y);
    }
    rt::place_mesh(*mesh, height, base_center);
    return rt::make_shared<rt::bvh_node>(mesh->objects, 0, mesh->objects.size());
}

} // namespace scenes
