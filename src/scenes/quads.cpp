#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void quads(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto left_red     = make_shared<rt::lambertian>(rt::color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<rt::lambertian>(rt::color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<rt::lambertian>(rt::color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<rt::lambertian>(rt::color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<rt::lambertian>(rt::color(0.2, 0.8, 0.8));

    world.add(make_shared<rt::quad>(rt::point3f(-3, -2, 5), rt::vec3f(0, 0, -4), rt::vec3f(0, 4, 0),
                                    left_red));
    world.add(make_shared<rt::quad>(rt::point3f(-2, -2, 0), rt::vec3f(4, 0, 0), rt::vec3f(0, 4, 0),
                                    back_green));
    world.add(make_shared<rt::quad>(rt::point3f(3, -2, 1), rt::vec3f(0, 0, 4), rt::vec3f(0, 4, 0),
                                    right_blue));
    world.add(make_shared<rt::quad>(rt::point3f(-2, 3, 1), rt::vec3f(4, 0, 0), rt::vec3f(0, 0, 4),
                                    upper_orange));
    world.add(make_shared<rt::quad>(rt::point3f(-2, -3, 5), rt::vec3f(4, 0, 0), rt::vec3f(0, 0, -4),
                                    lower_teal));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.70f, 0.80f, 1.00f);

    camera.field_of_view = 80;
    camera.look_from     = rt::point3f(0, 0, 9);
    camera.look_at       = rt::point3f(0, 0, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);

    camera.defocus_angle = 0;

    camera.output_path = output_path;
    camera.render(world);
}

} // namespace scenes
