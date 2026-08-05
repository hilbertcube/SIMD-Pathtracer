#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// Cornell box with Suzanne and the teapot on two turned cubes and the cow on the floor
void multiple_models(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    // Cornell box
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

    // A cube of the given size standing on the floor and turned by `degrees`; returns the middle
    // of its top face, where a model can be placed
    auto add_cube = [&](float center_x, float center_z, float size, float degrees)
    {
        const float half = size / 2;
        shared_ptr<rt::hittable> cube =
            rt::box(rt::point3f(-half, 0, -half), rt::point3f(half, size, half), white);
        cube = make_shared<rt::rotate_y>(cube, degrees);
        cube = make_shared<rt::translate>(cube, rt::vec3f(center_x, 0, center_z));
        world.add(cube);
        return rt::point3f(center_x, size, center_z);
    };

    auto yellow      = make_shared<rt::lambertian>(rt::color(0.9, 0.8, 0));
    auto suzanne_top = add_cube(385, 355, 165, 15.0f);
    world.add(load_model("models/suzanne.obj", yellow, 95, suzanne_top, 200.0f));

    auto grey       = make_shared<rt::lambertian>(rt::color(0.8, 0.8, 0.8));
    auto teapot_top = add_cube(185, 170, 165, -18.0f);
    world.add(load_model("models/teapot.obj", grey, 90, teapot_top, 20.0f));

    auto cyan = make_shared<rt::lambertian>(rt::color(0, 0.8, 0.9));
    world.add(load_model("models/spot.obj", cyan, 100, rt::point3f(420, 0, 70), -35.0f));

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130, 0, 0),
                                     rt::vec3f(0, 0, -105), shared_ptr<rt::material>()));

    rt::Camera camera;
    camera.aspect_ratio      = 1.0f;
    camera.image_width       = 600;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);
    camera.field_of_view     = 40;
    camera.look_from         = rt::point3f(278, 278, -800);
    camera.look_at           = rt::point3f(278, 278, 0);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.output_path       = output_path;
    camera.defocus_angle     = 0;
    camera.render(world, lights);
}

} // namespace scenes
