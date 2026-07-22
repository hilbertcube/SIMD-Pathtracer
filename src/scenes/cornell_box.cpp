#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void cornell_box(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto light = make_shared<rt::diffuse_light>(rt::color(15, 15, 15));

    world.add(make_shared<rt::quad>(rt::point3f(555, 0, 0), rt::vec3f(0, 555, 0),
                                    rt::vec3f(0, 0, 555), green));
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 0), rt::vec3f(0, 555, 0),
                                    rt::vec3f(0, 0, 555), red));
    world.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130, 0, 0),
                                    rt::vec3f(0, 0, -105), light));
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 0), rt::vec3f(555, 0, 0),
                                    rt::vec3f(0, 0, 555), white));
    world.add(make_shared<rt::quad>(rt::point3f(555, 555, 555), rt::vec3f(-555, 0, 0),
                                    rt::vec3f(0, 0, -555), white));
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 555), rt::vec3f(555, 0, 0),
                                    rt::vec3f(0, 555, 0), white));

    // left box
    shared_ptr<rt::hittable> tall_box =
        box(rt::point3f(0, 0, 0), rt::point3f(165, 330, 165), white);
    tall_box = make_shared<rt::rotate_y>(tall_box, 15.0f);
    tall_box = make_shared<rt::translate>(tall_box, rt::vec3f(265, 0, 295));

    // right box
    shared_ptr<rt::hittable> short_box =
        box(rt::point3f(0, 0, 0), rt::point3f(165, 165, 165), white);
    short_box = make_shared<rt::rotate_y>(short_box, -18.0f);
    short_box = make_shared<rt::translate>(short_box, rt::vec3f(130, 0, 65));

    // sphere
    auto brown_material = make_shared<rt::lambertian>(rt::color(0.4f, 0.2f, 0.1f));
    world.add(make_shared<rt::sphere>(rt::point3f(200, 215, 200), 50.0f, brown_material));

    // Add pyramid in the center-left area
    auto pyramid_material                  = make_shared<rt::lambertian>(rt::color(.12, .45, .5));
    shared_ptr<rt::hittable> pyramid_shape = pyramid(rt::point3f(325, 0, 100),   // base min corner
                                                     rt::point3f(475, 0, 200),   // base max corner
                                                     rt::point3f(400, 150, 150), // apex point
                                                     pyramid_material);

    world.add(tall_box);
    world.add(short_box);
    world.add(pyramid_shape);

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130, 0, 0),
                                     rt::vec3f(0, 0, -105), shared_ptr<rt::material>()));

    rt::Camera camera;

    camera.aspect_ratio      = 1.0f;
    camera.image_width       = 600;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);

    camera.field_of_view = 40;
    camera.look_from     = rt::point3f(278, 278, -800);
    camera.look_at       = rt::point3f(278, 278, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);
    camera.output_path   = output_path;

    camera.defocus_angle = 0;

    camera.render(world, lights);
}

} // namespace scenes
