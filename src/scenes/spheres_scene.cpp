#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void spheres_scene(const std::filesystem::path& output_path)
{
    rt::hittable_list world;

    auto ground_texture =
        make_shared<rt::checker_texture>(0.32f,                        // scale
                                         rt::color(0.5f, 0.5f, 0.5f),  // odd color
                                         rt::color(0.9f, 0.9f, 0.9f)); // even color
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f, -1000.0f, 0.0f), 1000.0f,
                                      make_shared<rt::lambertian>(ground_texture)));

    for (int grid_x = -11; grid_x < 11; grid_x++)
    {
        for (int grid_z = -11; grid_z < 11; grid_z++)
        {
            float material_choice = rt::random_float();
            rt::point3f center(grid_x + 0.9f * rt::random_float(), 0.2f,
                               grid_z + 0.9f * rt::random_float());

            if ((center - rt::point3f(4.0f, 0.2f, 0.0f)).length() > 0.9f)
            {
                shared_ptr<rt::material> sphere_material;

                if (material_choice < 0.8f)
                {
                    // diffuse
                    auto albedo     = rt::color::random() * rt::color::random();
                    sphere_material = make_shared<rt::lambertian>(albedo);
                    auto center2    = center + rt::vec3f(0.0f, rt::random_float(0.0f, 0.5f), 0.0f);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                }
                else if (material_choice < 0.95f)
                {
                    // metal
                    auto albedo     = rt::color::random(0.5f, 1.0f);
                    auto fuzz       = rt::random_float(0.5f, 1.0f);
                    sphere_material = make_shared<rt::metal>(albedo, fuzz);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                }
                else
                {
                    // glass
                    sphere_material = make_shared<rt::dielectrics>(1.5f);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                }
            }
        }
    }

    // three big spheres
    auto glass_material = make_shared<rt::dielectrics>(1.5f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, 1, 0), 1.0f, glass_material));

    auto brown_material = make_shared<rt::lambertian>(rt::color(0.4f, 0.2f, 0.1f));
    world.add(make_shared<rt::sphere>(rt::point3f(-4, 1, 0), 1.0f, brown_material));

    auto metal_material = make_shared<rt::metal>(rt::color(0.7f, 0.6f, 0.5f), 0.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(4, 1, 0), 1.0f, metal_material));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.70f, 0.80f, 1.00f);

    camera.field_of_view  = 20;
    camera.look_from      = rt::point3f(13, 2, 3);
    camera.look_at        = rt::point3f(0, 0, 0);
    camera.view_up        = rt::vec3f(0, 1, 0);
    camera.defocus_angle  = 0.6f;
    camera.focus_distance = 10.0f;
    camera.output_path    = output_path;
    camera.render(world);
}

} // namespace scenes
