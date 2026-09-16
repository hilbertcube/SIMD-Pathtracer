#include "scenes/scenes.h"

#include <functional>
#include <string>

#include "rt/geometry/mesh.h"
#include "rt/materials/advanced_materials.h"
#include "rt/ray_tracer.h"
#include "scenes/load_obj_by_material.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

namespace
{

// The kitchen's materials, chosen by the names Blender gave them. The model's own material file
// isn't used, so the named ones get a fitting look and the rest (appliances, dishes, tools) are
// spread over a small palette of kitchen materials, always the same one for the same name.
shared_ptr<rt::material> kitchen_material(const std::string& name)
{
    if (name == "walls" || name == "Material")
    {
        return make_shared<rt::lambertian>(rt::color(0.82, 0.78, 0.70));
    }
    if (name == "Material.012") // the floor
    {
        return make_shared<rt::oren_nayar>(rt::color(0.36, 0.32, 0.28), 0.5f);
    }
    if (name == "Material.004") // the window panes
    {
        return make_shared<rt::dielectrics>(1.5f);
    }
    if (name == "wooden_panel" || name == "wood" || name == "wooden_seat")
    {
        return make_shared<rt::coated>(rt::color(0.38, 0.22, 0.11));
    }
    if (name == "marble")
    {
        return make_shared<rt::coated>(rt::color(0.82, 0.82, 0.84));
    }
    if (name == "porcelain")
    {
        return make_shared<rt::coated>(rt::color(0.88, 0.88, 0.86));
    }
    if (name == "black" || name == "wall_black.001")
    {
        return make_shared<rt::coated>(rt::color(0.02, 0.02, 0.025));
    }
    if (name == "leaves_alpha_masked")
    {
        return make_shared<rt::lambertian>(rt::color(0.12, 0.35, 0.10));
    }

    const shared_ptr<rt::material> palette[] = {
        make_shared<rt::conductor>(rt::color(0.78, 0.79, 0.81), 0.25f), // stainless steel
        make_shared<rt::conductor>(rt::color(0.95, 0.70, 0.32), 0.30f), // brass
        make_shared<rt::coated>(rt::color(0.85, 0.83, 0.78)),           // cream ceramic
        make_shared<rt::coated>(rt::color(0.08, 0.08, 0.09)),           // black plastic
        make_shared<rt::coated>(rt::color(0.55, 0.30, 0.15)),           // light wood
        make_shared<rt::oren_nayar>(rt::color(0.70, 0.68, 0.62), 0.5f), // stoneware
        make_shared<rt::coated>(rt::color(0.65, 0.12, 0.10)),           // red enamel
        make_shared<rt::conductor>(rt::color(0.55, 0.56, 0.58), 0.40f), // brushed metal
    };
    return palette[std::hash<std::string>()(name) % (sizeof(palette) / sizeof(palette[0]))];
}

} // namespace

// A wooden kitchen with a marble island, lit by a window and the ceiling lamps
void kitchen(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights; // what the shadow rays aim at: geometry only, materials are ignored

    // The model is in meters; the scene is in centimeters
    auto kitchen_mesh = load_obj_by_material("models/new+kitchen.obj", kitchen_material);
    rt::transform_mesh(*kitchen_mesh, 100.0f, rt::vec3f(0, 0, 0));
    world.add(make_shared<rt::bvh_node>(kitchen_mesh->objects, 0, kitchen_mesh->objects.size()));

    auto add_light = [&](const rt::point3f& corner, const rt::vec3f& edge_u,
                         const rt::vec3f& edge_v, const rt::color& emission, int weight)
    {
        world.add(make_shared<rt::quad>(corner, edge_u, edge_v,
                                        make_shared<rt::diffuse_light>(emission)));
        for (int i = 0; i < weight; ++i)
        {
            lights.add(make_shared<rt::quad>(corner, edge_u, edge_v, shared_ptr<rt::material>()));
        }
    };

    // Daylight outside the windows in the back wall
    add_light(rt::point3f(-120, 0, -350), rt::vec3f(480, 0, 0), rt::vec3f(0, 320, 0),
              rt::color(5.0f, 5.3f, 6.0f), 3);

    // Three pendant lamps hanging over the island
    auto cord = make_shared<rt::lambertian>(rt::color(0.02, 0.02, 0.02));
    for (float x : {30.0f, 140.0f, 250.0f})
    {
        const rt::point3f bulb(x, 190, -48);
        world.add(make_shared<rt::sphere>(
            bulb, 9, make_shared<rt::diffuse_light>(rt::color(70.0f, 52.0f, 32.0f))));
        lights.add(make_shared<rt::sphere>(bulb, 9, shared_ptr<rt::material>()));
        world.add(
            rt::box(rt::point3f(x - 0.4f, 199, -48.4f), rt::point3f(x + 0.4f, 300, -47.6f), cord));
    }

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.7, 0.7, 0.7);
    camera.field_of_view     = 58;
    camera.look_from         = rt::point3f(-170, 150, 215);
    camera.look_at           = rt::point3f(125, 105, -110);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
