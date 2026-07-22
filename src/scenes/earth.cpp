#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void earth(const std::filesystem::path& output_path)
{
    auto earth_texture = make_shared<rt::image_texture>("texture/earth2048.bmp");
    auto earth_surface = make_shared<rt::lambertian>(earth_texture);
    auto globe = make_shared<rt::sphere>(rt::point3f(0.0f, 0.0f, 0.0f), 2.0f, earth_surface);

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.70f, 0.80f, 1.00f);

    camera.field_of_view = 20;
    camera.look_from     = rt::point3f(0, 0, 12);
    camera.look_at       = rt::point3f(0, 0, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);
    camera.defocus_angle = 0.0f;

    camera.output_path = output_path;
    camera.render(rt::hittable_list(globe));
}

} // namespace scenes
