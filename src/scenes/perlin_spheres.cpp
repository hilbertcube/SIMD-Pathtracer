#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void perlin_spheres(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto marble_texture = make_shared<rt::noise_texture>(4.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, -1000, 0), 1000.0f,
                                      make_shared<rt::lambertian>(marble_texture)));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 2, 0), 2.0f,
                                      make_shared<rt::lambertian>(marble_texture)));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.70f, 0.80f, 1.00f);
    camera.field_of_view     = 20;
    camera.look_from         = rt::point3f(13, 2, 3);
    camera.look_at           = rt::point3f(0, 0, 0);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0.0f;
    camera.output_path       = output_path;

    camera.render(world);
}

} // namespace scenes
