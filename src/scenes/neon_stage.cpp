#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// A chrome armadillo and a glass bunny lit only by neon tubes, over a glossy black floor
void neon_stage(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights;

    auto floor   = make_shared<rt::metal>(rt::color(0.08, 0.08, 0.09), 0.03f);
    auto wall    = make_shared<rt::lambertian>(rt::color(0.05, 0.05, 0.06));
    auto magenta = make_shared<rt::diffuse_light>(rt::color(9, 0.6, 6));
    auto cyan    = make_shared<rt::diffuse_light>(rt::color(0.4, 6, 9));

    world.add(make_shared<rt::quad>(rt::point3f(-1000, 0, -1000), rt::vec3f(2000, 0, 0),
                                    rt::vec3f(0, 0, 1600), floor));
    world.add(make_shared<rt::quad>(rt::point3f(-1000, 0, 600), rt::vec3f(2000, 0, 0),
                                    rt::vec3f(0, 800, 0), wall));

    // A neon tube: a thin panel facing the camera (-z); edge_u x edge_v points to -z
    auto add_tube = [&](const rt::point3f& corner, const rt::vec3f& edge_u, const rt::vec3f& edge_v,
                        shared_ptr<rt::material> color)
    {
        world.add(make_shared<rt::quad>(corner, edge_u, edge_v, color));
        lights.add(make_shared<rt::quad>(corner, edge_u, edge_v, shared_ptr<rt::material>()));
    };

    // Vertical tubes on the back wall, alternating colors
    for (int i = 0; i < 6; ++i)
    {
        float x = -375 + 150 * i;
        add_tube(rt::point3f(x, 40, 590), rt::vec3f(0, 420, 0), rt::vec3f(8, 0, 0),
                 i % 2 == 0 ? magenta : cyan);
    }

    // Two panels standing on the floor at the sides, facing inward (edge_u x edge_v toward x = 0)
    add_tube(rt::point3f(-330, 0, 120), rt::vec3f(0, 300, 0), rt::vec3f(0, 0, 260), cyan);
    add_tube(rt::point3f(330, 0, 120), rt::vec3f(0, 0, 260), rt::vec3f(0, 300, 0), magenta);

    auto chrome = make_shared<rt::metal>(rt::color(0.92, 0.92, 0.95), 0.03f);
    auto glass  = make_shared<rt::dielectrics>(1.5f);
    world.add(load_model("model/armadillo.obj", chrome, 300, rt::point3f(-40, 0, 260), 15.0f));
    world.add(load_model("model/stanford-bunny.obj", glass, 110, rt::point3f(170, 0, 120), -60.0f));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);
    camera.field_of_view     = 38;
    camera.look_from         = rt::point3f(0, 170, -700);
    camera.look_at           = rt::point3f(0, 150, 200);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
