#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void simple_light(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto marble_texture = make_shared<rt::noise_texture>(4.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, -1000, 0), 1000.0f,
                                      make_shared<rt::lambertian>(marble_texture)));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 2, 0), 2.0f,
                                      make_shared<rt::lambertian>(marble_texture)));

    auto light_material = make_shared<rt::diffuse_light>(rt::color(4, 4, 4));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 7, 0), 2.0f, light_material));
    world.add(make_shared<rt::quad>(rt::point3f(3, 1, -2), rt::vec3f(2, 0, 0), rt::vec3f(0, 2, 0),
                                    light_material));

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::sphere>(rt::point3f(0, 7, 0), 2.0f, shared_ptr<rt::material>()));
    lights.add(make_shared<rt::quad>(rt::point3f(3, 1, -2), rt::vec3f(2, 0, 0), rt::vec3f(0, 2, 0),
                                     shared_ptr<rt::material>()));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);

    camera.field_of_view = 20;
    camera.look_from     = rt::point3f(26, 3, 6);
    camera.look_at       = rt::point3f(0, 2, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);
    camera.defocus_angle = 0;

    camera.output_path = output_path;
    camera.render(world, lights);
}

} // namespace scenes
