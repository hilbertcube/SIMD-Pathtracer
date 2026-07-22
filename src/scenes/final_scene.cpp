#include "scenes/scenes.h"

#include "rt/ray_tracer.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void final_scene(int image_width, int samples_per_pixel, int max_depth,
                 const std::filesystem::path& output_path)
{
    rt::hittable_list ground_boxes;
    auto ground_material = make_shared<rt::lambertian>(rt::color(0.48, 0.83, 0.53));

    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++)
    {
        for (int j = 0; j < boxes_per_side; j++)
        {
            auto box_width = 100.0f;
            auto min_x     = -1000.0f + i * box_width;
            auto min_z     = -1000.0f + j * box_width;
            auto min_y     = 0.0f;
            auto max_x     = min_x + box_width;
            auto max_y     = rt::random_float(1.0f, 101.0f);
            auto max_z     = min_z + box_width;

            ground_boxes.add(rt::box(rt::point3f(min_x, min_y, min_z),
                                     rt::point3f(max_x, max_y, max_z), ground_material));
        }
    }

    rt::hittable_list world;

    world.add(make_shared<rt::bvh_node>(ground_boxes));

    // top light
    auto light = make_shared<rt::diffuse_light>(rt::color(7, 7, 7));
    world.add(make_shared<rt::quad>(rt::point3f(123, 554, 147), rt::vec3f(300, 0, 0),
                                    rt::vec3f(0, 0, 265), light));

    // moving sphere
    auto center1         = rt::point3f(400, 400, 200);
    auto center2         = center1 + rt::vec3f(30, 0, 0);
    auto sphere_material = make_shared<rt::lambertian>(rt::color(0.7, 0.3, 0.1));
    world.add(make_shared<rt::sphere>(center1, center2, 50.0f, sphere_material));

    // dielectrics sphere
    world.add(make_shared<rt::sphere>(rt::point3f(260, 150, 45), 50.0f,
                                      make_shared<rt::dielectrics>(1.5f)));

    // metal sphere
    world.add(make_shared<rt::sphere>(rt::point3f(0, 150, 145), 50.0f,
                                      make_shared<rt::metal>(rt::color(0.8, 0.8, 0.9), 1.0f)));

    auto boundary = make_shared<rt::sphere>(rt::point3f(360, 150, 145), 70.0f,
                                            make_shared<rt::dielectrics>(1.5f));
    world.add(boundary);
    world.add(make_shared<rt::constant_medium>(boundary, 0.2f, rt::color(0.2, 0.4, 0.9)));
    boundary =
        make_shared<rt::sphere>(rt::point3f(0, 0, 0), 5000.0f, make_shared<rt::dielectrics>(1.5f));
    world.add(make_shared<rt::constant_medium>(boundary, 0.0001f, rt::color(1, 1, 1)));

    auto earth_material =
        make_shared<rt::lambertian>(make_shared<rt::image_texture>("texture/earth2048.bmp"));
    world.add(make_shared<rt::sphere>(rt::point3f(400, 200, 400), 100.0f, earth_material));
    auto marble_texture = make_shared<rt::noise_texture>(0.2f);
    world.add(make_shared<rt::sphere>(rt::point3f(220, 280, 300), 80.0f,
                                      make_shared<rt::lambertian>(marble_texture)));

    rt::hittable_list sphere_cluster;
    auto white       = make_shared<rt::lambertian>(rt::color(0.73, 0.73, 0.73));
    int sphere_count = 1000;
    for (int j = 0; j < sphere_count; j++)
    {
        sphere_cluster.add(make_shared<rt::sphere>(rt::point3f::random(0, 165), 10.0f, white));
    }

    world.add(make_shared<rt::translate>(
        make_shared<rt::rotate_y>(make_shared<rt::bvh_node>(sphere_cluster), 15.0f),
        rt::vec3f(-100, 270, 395)));

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::quad>(rt::point3f(123, 554, 147), rt::vec3f(300, 0, 0),
                                     rt::vec3f(0, 0, 265), shared_ptr<rt::material>()));

    rt::Camera camera;

    camera.aspect_ratio      = 1.0f;
    camera.image_width       = image_width;
    camera.samples_per_pixel = samples_per_pixel;
    camera.max_depth         = max_depth;
    camera.background        = rt::color(0, 0, 0);

    camera.field_of_view = 40;
    camera.look_from     = rt::point3f(478, 278, -600);
    camera.look_at       = rt::point3f(278, 278, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);

    camera.defocus_angle = 0;
    camera.output_path   = output_path;

    camera.render(world, lights);
}

} // namespace scenes
