#include "scenes/scenes.h"

#include "rt/materials/materials.h"
#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// A gold dragon (a 250k-triangle model) on a stone pedestal, with its reflection in a mirror
// floor, in front of a red wall
void dragon(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights; // what the shadow rays aim at: geometry only, materials are ignored

    // A dark mirror floor, so the dragon is reflected below itself
    world.add(make_shared<rt::quad>(rt::point3f(-2000, 0, -1400), rt::vec3f(4000, 0, 0),
                                    rt::vec3f(0, 0, 2800),
                                    make_shared<rt::metal>(rt::color(0.55, 0.55, 0.60), 0.0f)));

    // A backdrop in the Cornell box's red
    auto wall = make_shared<rt::lambertian>(rt::color(0.65, 0.05, 0.05));
    world.add(make_shared<rt::quad>(rt::point3f(-2000, 0, 700), rt::vec3f(4000, 0, 0),
                                    rt::vec3f(0, 1000, 0), wall));

    // Lights: a big softbox overhead, strips at the sides for long reflections on the gold (out of
    // the picture), and a dim fill behind the camera. Each is also a target for the shadow rays.
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
    add_light(rt::point3f(-600, 700, -300), rt::vec3f(1200, 0, 0), rt::vec3f(0, 0, 600),
              rt::color(1.9f, 1.9f, 1.9f), 3);
    add_light(rt::point3f(-820, 20, -250), rt::vec3f(0, 420, 0), rt::vec3f(0, 0, 400),
              rt::color(0.8f, 0.8f, 0.9f), 1);
    add_light(rt::point3f(820, 20, -250), rt::vec3f(0, 0, 400), rt::vec3f(0, 420, 0),
              rt::color(0.8f, 0.8f, 0.9f), 1);
    add_light(rt::point3f(-600, 100, -1150), rt::vec3f(1200, 0, 0), rt::vec3f(0, 400, 0),
              rt::color(0.5f, 0.5f, 0.5f), 1);

    // The dragon, turned a little toward the camera, on a pedestal
    const float pedestal_height = 50;
    world.add(rt::box(rt::point3f(-320, 0, -190), rt::point3f(320, pedestal_height, 190),
                      make_shared<rt::coated>(rt::color(0.55, 0.55, 0.58))));

    auto gold = make_shared<rt::conductor>(rt::color(1.0, 0.78, 0.34), 0.25f);
    world.add(
        load_model("models/dragon.obj", gold, 280, rt::point3f(0, pedestal_height, 0), 25.0f));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 4.0f / 3.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.02, 0.02, 0.03);
    camera.field_of_view     = 38;
    camera.look_from         = rt::point3f(0, 260, -1100);
    camera.look_at           = rt::point3f(0, 5, 0);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
