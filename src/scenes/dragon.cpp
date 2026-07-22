#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "rt/geometry/mesh.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// Dragon model in the Cornell box, with mirrors on the side and back walls
void dragon(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    // Cornell box
    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto blue  = make_shared<rt::lambertian>(rt::color(0.4, 0.6, 0.9));
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
                                    rt::vec3f(0, 555, 0), blue));

    // Mirrors, set slightly in front of the walls to avoid z-fighting
    auto mirror = make_shared<rt::metal>(rt::color(0.95, 0.95, 0.95), 0.0f);
    world.add(make_shared<rt::quad>(rt::point3f(555 - 0.01f, 50, 50), rt::vec3f(0, 455, 0),
                                    rt::vec3f(0, 0, 455), mirror)); // green wall (x = 555)
    world.add(make_shared<rt::quad>(rt::point3f(0 + 0.01f, 50, 50), rt::vec3f(0, 455, 0),
                                    rt::vec3f(0, 0, 455), mirror)); // red wall (x = 0)
    world.add(make_shared<rt::quad>(rt::point3f(50, 50, 555 - 0.01f), rt::vec3f(455, 0, 0),
                                    rt::vec3f(0, 455, 0), mirror)); // back wall (z = 555)

    // Dragon model
    auto dragon_material = make_shared<rt::metal>(rt::color(0.95, 0.75, 0.30), 0.1f);
    auto dragon_mesh     = rt::load_obj("model/dragon.obj", dragon_material);
    rt::transform_mesh(*dragon_mesh, 2.4f, rt::vec3f(278, 100, 210));
    world.add(make_shared<rt::bvh_node>(dragon_mesh->objects, 0, dragon_mesh->objects.size()));

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130, 0, 0),
                                     rt::vec3f(0, 0, -105), shared_ptr<rt::material>()));

    rt::Camera camera;
    camera.aspect_ratio      = 1.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 3000; // light bouncing off the mirrors is slow to converge
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
