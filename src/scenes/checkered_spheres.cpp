#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void checkered_spheres(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto checker = make_shared<rt::checker_texture>(0.32f, rt::color(0.2f, 0.3f, 0.1f),
                                                    rt::color(0.9f, 0.9f, 0.9f));
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f, -10.0f, 0.0f), 10.0f,
                                      make_shared<rt::lambertian>(checker)));
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f, 10.0f, 0.0f), 10.0f,
                                      make_shared<rt::lambertian>(checker)));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.70f, 0.80f, 1.00f);

    camera.field_of_view = 20;
    camera.look_from     = rt::point3f(13, 2, 3);
    camera.look_at       = rt::point3f(0, 0, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);
    camera.defocus_angle = 0;
    camera.output_path   = output_path;

    camera.render(world);
}

} // namespace scenes
