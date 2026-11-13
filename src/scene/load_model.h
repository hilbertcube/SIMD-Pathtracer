#pragma once

#include <string>

#include "rt/acceleration/bvh_node.h"
#include "rt/geometry/mesh.h"
#include "rt/materials/materials.h"

namespace scenes
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

// Load a model with a single material and place it (see place_model)
inline rt::shared_ptr<rt::hittable> load_model(const std::string& filename,
                                               rt::shared_ptr<rt::material> material, float height,
                                               const rt::point3f& base_center, float turn_y = 0.0f,
                                               float tilt_x = 0.0f)
{
    auto mesh = rt::load_obj(filename, material);
    return place_model(*mesh, height, base_center, turn_y, tilt_x);
}

/*
 * Load a model that has texture coordinates and normals and place it (see place_model). With a
 * textured material, the model shows its own colors, and curved parts shade smoothly.
 */
inline rt::shared_ptr<rt::hittable> load_textured_model(const std::string& filename,
                                                        rt::shared_ptr<rt::material> material,
                                                        float height,
                                                        const rt::point3f& base_center,
                                                        float turn_y = 0.0f, float tilt_x = 0.0f)
{
    auto mesh = rt::load_obj_with_attributes(filename, material);
    return place_model(*mesh, height, base_center, turn_y, tilt_x);
}

/*
 * Load a model, give it smooth shading computed from its geometry if it has no normals, and
 * place it (see place_model). For low-poly models that would look faceted otherwise.
 */
inline rt::shared_ptr<rt::hittable> load_smooth_model(const std::string& filename,
                                                      rt::shared_ptr<rt::material> material,
                                                      float height, const rt::point3f& base_center,
                                                      float turn_y = 0.0f, float tilt_x = 0.0f)
{
    auto mesh = rt::load_obj_smooth(filename, material);
    return place_model(*mesh, height, base_center, turn_y, tilt_x);
}

// The vertex of a placed mesh that lies farthest along `direction`
inline rt::point3f frontmost_vertex(const rt::hittable_list& mesh, const rt::vec3f& direction)
{
    rt::point3f best;
    float best_distance = -rt::INF;
    for (const auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<rt::mesh_triangle>(object);
        if (!face)
        {
            continue;
        }
        for (const rt::point3f* vertex : {&face->vertex0, &face->vertex1, &face->vertex2})
        {
            float distance = dot(direction, *vertex);
            if (distance > best_distance)
            {
                best_distance = distance;
                best          = *vertex;
            }
        }
    }
    return best;
}

} // namespace scenes
