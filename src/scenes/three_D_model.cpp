#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "rt/geometry/mesh.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void three_D_model(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    // Cornell box
    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto light = make_shared<rt::diffuse_light>(rt::color(15, 15, 15));

    auto blue = make_shared<rt::lambertian>(rt::color(0.4, 0.6, 0.9));

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
                                    rt::vec3f(0, 555, 0), blue));

    // Tea pot
    auto teapot_material = make_shared<rt::metal>(rt::color(0, 0, 0.8), 0.0f);
    auto teapot          = rt::load_obj("model/teapot.obj", teapot_material);
    rt::transform_mesh(*teapot, 80.0f, rt::vec3f(278, 0, 278));
    auto teapot_bvh = make_shared<rt::bvh_node>(teapot->objects, 0, teapot->objects.size());
    world.add(teapot_bvh);

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130, 0, 0),
                                     rt::vec3f(0, 0, -105), shared_ptr<rt::material>()));

    rt::Camera camera;
    camera.aspect_ratio      = 1.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 20;
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
