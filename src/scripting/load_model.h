#pragma once

#include "rt/acceleration/bvh_node.h"
#include "rt/geometry/mesh.h"

namespace scripting
{

/*
 * Stand a loaded mesh `height` tall with the middle of its base at `base_center`, and return it
 * in its own BVH. `tilt_x` turns it about x first (for models stored Z-up), then `turn_y` turns
 * it about the vertical axis to face the camera.
 */
inline rt::shared_ptr<rt::hittable> place_model(rt::hittable_list& mesh, float height,
                                                const rt::point3f& base_center, float turn_y,
                                                float tilt_x)
{
    if (tilt_x != 0.0f)
    {
        rt::rotate_mesh(mesh, 0, tilt_x);
    }
    if (turn_y != 0.0f)
    {
        rt::rotate_mesh(mesh, 1, turn_y);
    }
    rt::place_mesh(mesh, height, base_center);
    return rt::make_shared<rt::bvh_node>(mesh.objects, 0, mesh.objects.size());
}

} // namespace scripting
