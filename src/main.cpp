#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

#include "rt/ray_tracer.h"
#include "rt/geometry/mesh.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

/*
 * Load a model, stand it `height` tall with the middle of its base at `base_center`, and
 * return it in its own BVH. `tilt_x` turns about x first (for models stored Z-up), then
 * `turn_y` turns it about the vertical axis to face the camera.
 */
shared_ptr<rt::hittable> load_model(const std::string& filename, shared_ptr<rt::material> material,
                                    float height, const rt::point3f& base_center,
                                    float turn_y = 0.0f, float tilt_x = 0.0f)
{
    auto mesh = rt::load_obj(filename, material);
    if (tilt_x != 0.0f)
    {
        rt::rotate_mesh(*mesh, 0, tilt_x);
    }
    if (turn_y != 0.0f)
    {
        rt::rotate_mesh(*mesh, 1, turn_y);
    }
    rt::place_mesh(*mesh, height, base_center);
    return make_shared<rt::bvh_node>(mesh->objects, 0, mesh->objects.size());
}

void spheres_scene(const std::filesystem::path& output_directory)
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

    camera.field_of_view    = 20;
    camera.look_from        = rt::point3f(13, 2, 3);
    camera.look_at          = rt::point3f(0, 0, 0);
    camera.view_up          = rt::vec3f(0, 1, 0);
    camera.defocus_angle    = 0.6f;
    camera.focus_distance   = 10.0f;
    camera.output_filename  = "spheres.png";
    camera.output_directory = output_directory;
    camera.render(world);
}

void checkered_spheres(const std::filesystem::path& output_directory)
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

    camera.field_of_view    = 20;
    camera.look_from        = rt::point3f(13, 2, 3);
    camera.look_at          = rt::point3f(0, 0, 0);
    camera.view_up          = rt::vec3f(0, 1, 0);
    camera.defocus_angle    = 0;
    camera.output_filename  = "checked_spheres.png";
    camera.output_directory = output_directory;

    camera.render(world);
}

void earth(const std::filesystem::path& output_directory)
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

    camera.output_filename  = "earth.png";
    camera.output_directory = output_directory;
    camera.render(rt::hittable_list(globe));
}

void perlin_spheres(const std::filesystem::path& output_directory)
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
    camera.output_filename   = "perlin_spheres.png";
    camera.output_directory  = output_directory;

    camera.render(world);
}

void quads(const std::filesystem::path& output_directory)
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

    camera.output_filename  = "quads.png";
    camera.output_directory = output_directory;
    camera.render(world);
}

void simple_light(const std::filesystem::path& output_directory)
{
    rt::hittable_list world;

    auto marble_texture = make_shared<rt::noise_texture>(4.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, -1000, 0), 1000.0f,
                                      make_shared<rt::lambertian>(marble_texture)));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 2, 0), 2.0f,
                                      make_shared<rt::lambertian>(marble_texture)));

    auto light_material = make_shared<rt::diffuse_light>(rt::color(4, 4, 4));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 7, 0), 2.0f, light_material));
    world.add(make_shared<rt::quad>(rt::point3f(3, 1, -2), rt::vec3f(2, 0, 0), rt::vec3f(0, 2, 0),
                                    light_material));

    // Top-level BVH
    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    // Lights to importance-sample
    rt::hittable_list lights;
    lights.add(make_shared<rt::sphere>(rt::point3f(0, 7, 0), 2.0f, shared_ptr<rt::material>()));
    lights.add(make_shared<rt::quad>(rt::point3f(3, 1, -2), rt::vec3f(2, 0, 0), rt::vec3f(0, 2, 0),
                                     shared_ptr<rt::material>()));

    rt::Camera camera;

    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1000;
    camera.samples_per_pixel = 100;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);

    camera.field_of_view = 20;
    camera.look_from     = rt::point3f(26, 3, 6);
    camera.look_at       = rt::point3f(0, 2, 0);
    camera.view_up       = rt::vec3f(0, 1, 0);
    camera.defocus_angle = 0;

    camera.output_filename  = "simple_light.png";
    camera.output_directory = output_directory;
    camera.render(world, lights);
}

void cornell_box(const std::filesystem::path& output_directory)
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

    camera.field_of_view    = 40;
    camera.look_from        = rt::point3f(278, 278, -800);
    camera.look_at          = rt::point3f(278, 278, 0);
    camera.view_up          = rt::vec3f(0, 1, 0);
    camera.output_filename  = "cornell_box.png";
    camera.output_directory = output_directory;

    camera.defocus_angle = 0;

    camera.render(world, lights);
}

void final_scene(int image_width, int samples_per_pixel, int max_depth,
                 const std::filesystem::path& output_directory)
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

    camera.defocus_angle    = 0;
    camera.output_filename  = "final_scene.png";
    camera.output_directory = output_directory;

    camera.render(world, lights);
}

void three_D_model(const std::filesystem::path& output_directory)
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
    camera.output_filename   = "3d_model.png";
    camera.output_directory  = output_directory;
    camera.defocus_angle     = 0;
    camera.render(world, lights);
}

// Dragon model in the Cornell box, with mirrors on the side and back walls
void dragon(const std::filesystem::path& output_directory)
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
    camera.output_filename   = "dragon.png";
    camera.output_directory  = output_directory;
    camera.defocus_angle     = 0;
    camera.render(world, lights);
}

// Cornell box with Suzanne and the teapot on two turned cubes and the cow on the floor
void multiple_models(const std::filesystem::path& output_directory)
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
    world.add(load_model("model/suzanne.obj", yellow, 95, suzanne_top, 200.0f));

    auto grey       = make_shared<rt::lambertian>(rt::color(0.8, 0.8, 0.8));
    auto teapot_top = add_cube(185, 170, 165, -18.0f);
    world.add(load_model("model/teapot.obj", grey, 90, teapot_top, 20.0f));

    auto cyan = make_shared<rt::lambertian>(rt::color(0, 0.8, 0.9));
    world.add(load_model("model/spot.obj", cyan, 100, rt::point3f(420, 0, 70), -35.0f));

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
    camera.output_filename   = "multiple_3d_models.png";
    camera.output_directory  = output_directory;
    camera.defocus_angle     = 0;
    camera.render(world, lights);
}

// Three statues on marble pedestals under spotlights: stone Lucy, a gold Buddha, a bronze bust
void sculpture_gallery(const std::filesystem::path& output_directory)
{
    rt::hittable_list world;

    auto wall_paint = make_shared<rt::lambertian>(rt::color(0.45, 0.12, 0.10));
    auto ceiling    = make_shared<rt::lambertian>(rt::color(0.75, 0.75, 0.75));
    auto floor      = make_shared<rt::lambertian>(rt::color(0.30, 0.29, 0.28)); // polished stone
    auto marble     = make_shared<rt::lambertian>(rt::color(0.80, 0.78, 0.74));
    auto spotlight  = make_shared<rt::diffuse_light>(rt::color(18, 16, 13));

    // Room: x 0..900, y 0..500, z 0..600, open toward the camera
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 0), rt::vec3f(900, 0, 0),
                                    rt::vec3f(0, 0, 600), floor));
    world.add(make_shared<rt::quad>(rt::point3f(0, 500, 0), rt::vec3f(900, 0, 0),
                                    rt::vec3f(0, 0, 600), ceiling));
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 600), rt::vec3f(900, 0, 0),
                                    rt::vec3f(0, 500, 0), wall_paint));
    world.add(make_shared<rt::quad>(rt::point3f(0, 0, 0), rt::vec3f(0, 500, 0),
                                    rt::vec3f(0, 0, 600), wall_paint));
    world.add(make_shared<rt::quad>(rt::point3f(900, 0, 0), rt::vec3f(0, 500, 0),
                                    rt::vec3f(0, 0, 600), wall_paint));

    // Pedestals with a statue on each, and a spotlight panel above each
    const float pedestal_x[3]   = {200, 450, 700};
    const float pedestal_height = 140;
    rt::hittable_list lights;
    for (float x : pedestal_x)
    {
        world.add(rt::box(rt::point3f(x - 60, 0, 290), rt::point3f(x + 60, pedestal_height, 410),
                          marble));

        // Facing down: edge_u x edge_v points to -y
        rt::point3f corner(x - 45, 499, 305);
        world.add(
            make_shared<rt::quad>(corner, rt::vec3f(90, 0, 0), rt::vec3f(0, 0, 90), spotlight));
        lights.add(make_shared<rt::quad>(corner, rt::vec3f(90, 0, 0), rt::vec3f(0, 0, 90),
                                         shared_ptr<rt::material>()));
    }

    auto stone  = make_shared<rt::lambertian>(rt::color(0.85, 0.83, 0.78));
    auto gold   = make_shared<rt::metal>(rt::color(0.95, 0.72, 0.30), 0.2f);
    auto bronze = make_shared<rt::metal>(rt::color(0.55, 0.35, 0.20), 0.35f);
    world.add(load_model("model/lucy.obj", stone, 230, rt::point3f(200, pedestal_height, 350), 0.0f,
                         -90.0f));
    world.add(load_model("model/happy.obj", gold, 200, rt::point3f(450, pedestal_height, 350)));
    world.add(
        load_model("model/max-planck.obj", bronze, 190, rt::point3f(700, pedestal_height, 350)));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);
    camera.field_of_view     = 34;
    camera.look_from         = rt::point3f(450, 250, -650);
    camera.look_at           = rt::point3f(450, 220, 350);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_filename   = "sculpture_gallery.png";
    camera.output_directory  = output_directory;
    camera.render(world, lights);
}

// A chrome armadillo and a glass bunny lit only by neon tubes, over a glossy black floor
void neon_stage(const std::filesystem::path& output_directory)
{
    rt::hittable_list world;
    rt::hittable_list lights;

    auto floor   = make_shared<rt::metal>(rt::color(0.08, 0.08, 0.09), 0.03f);
    auto wall    = make_shared<rt::lambertian>(rt::color(0.05, 0.05, 0.06));
    auto magenta = make_shared<rt::diffuse_light>(rt::color(9, 0.6, 6));
    auto cyan    = make_shared<rt::diffuse_light>(rt::color(0.4, 6, 9));

    world.add(make_shared<rt::quad>(rt::point3f(-1000, 0, -1000), rt::vec3f(2000, 0, 0),
                                    rt::vec3f(0, 0, 1600), floor));
    world.add(make_shared<rt::quad>(rt::point3f(-1000, 0, 600), rt::vec3f(2000, 0, 0),
                                    rt::vec3f(0, 800, 0), wall));

    // A neon tube: a thin panel facing the camera (-z); edge_u x edge_v points to -z
    auto add_tube = [&](const rt::point3f& corner, const rt::vec3f& edge_u, const rt::vec3f& edge_v,
                        shared_ptr<rt::material> color)
    {
        world.add(make_shared<rt::quad>(corner, edge_u, edge_v, color));
        lights.add(make_shared<rt::quad>(corner, edge_u, edge_v, shared_ptr<rt::material>()));
    };

    // Vertical tubes on the back wall, alternating colors
    for (int i = 0; i < 6; ++i)
    {
        float x = -375 + 150 * i;
        add_tube(rt::point3f(x, 40, 590), rt::vec3f(0, 420, 0), rt::vec3f(8, 0, 0),
                 i % 2 == 0 ? magenta : cyan);
    }

    // Two panels standing on the floor at the sides, facing inward (edge_u x edge_v toward x = 0)
    add_tube(rt::point3f(-330, 0, 120), rt::vec3f(0, 300, 0), rt::vec3f(0, 0, 260), cyan);
    add_tube(rt::point3f(330, 0, 120), rt::vec3f(0, 0, 260), rt::vec3f(0, 300, 0), magenta);

    auto chrome = make_shared<rt::metal>(rt::color(0.92, 0.92, 0.95), 0.03f);
    auto glass  = make_shared<rt::dielectrics>(1.5f);
    world.add(load_model("model/armadillo.obj", chrome, 300, rt::point3f(-40, 0, 260), 15.0f));
    world.add(load_model("model/stanford-bunny.obj", glass, 110, rt::point3f(170, 0, 120), -60.0f));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);
    camera.field_of_view     = 38;
    camera.look_from         = rt::point3f(0, 170, -700);
    camera.look_at           = rt::point3f(0, 150, 200);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_filename   = "neon_stage.png";
    camera.output_directory  = output_directory;
    camera.render(world, lights);
}

// A ring of colorful bunnies around a glowing orb at night, with a horse watching from the dark
void bunny_campfire(const std::filesystem::path& output_directory)
{
    rt::hittable_list world;

    auto ground = make_shared<rt::lambertian>(make_shared<rt::checker_texture>(
        40.0f, rt::color(0.10, 0.16, 0.07), rt::color(0.05, 0.09, 0.04)));
    world.add(make_shared<rt::quad>(rt::point3f(-3000, 0, -3000), rt::vec3f(6000, 0, 0),
                                    rt::vec3f(0, 0, 6000), ground));

    // The fire: a glowing orb with a glass shell
    rt::point3f fire_center(0, 45, 0);
    auto fire = make_shared<rt::diffuse_light>(rt::color(34, 14, 3.5));
    world.add(make_shared<rt::sphere>(fire_center, 18, fire));
    world.add(make_shared<rt::sphere>(fire_center, 26, make_shared<rt::dielectrics>(1.3f)));

    rt::hittable_list lights;
    lights.add(make_shared<rt::sphere>(fire_center, 18, shared_ptr<rt::material>()));

    // Bunnies evenly spaced on a circle, each turned to face the fire (the model faces -x)
    const rt::color colors[7] = {rt::color(0.85, 0.20, 0.20), rt::color(0.95, 0.60, 0.15),
                                 rt::color(0.90, 0.85, 0.25), rt::color(0.30, 0.75, 0.30),
                                 rt::color(0.25, 0.55, 0.90), rt::color(0.55, 0.35, 0.85),
                                 rt::color(0.92, 0.92, 0.92)};
    const int bunny_count     = 7;
    const float ring_radius   = 160;
    for (int i = 0; i < bunny_count; ++i)
    {
        float angle = 2.0f * rt::PI * (i + 0.5f) / bunny_count;
        rt::point3f base(ring_radius * std::sin(angle), 0, ring_radius * std::cos(angle));
        float facing_degrees = rt::radians_to_degrees(angle) - 90.0f;
        world.add(load_model("model/stanford-bunny.obj", make_shared<rt::lambertian>(colors[i]), 70,
                             base, facing_degrees));
    }

    auto horse_coat = make_shared<rt::lambertian>(rt::color(0.45, 0.30, 0.18));
    world.add(
        load_model("model/horse.obj", horse_coat, 190, rt::point3f(-260, 0, 330), -30.0f, -90.0f));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.02, 0.025, 0.06); // moonlit night sky
    camera.field_of_view     = 36;
    camera.look_from         = rt::point3f(0, 230, -560);
    camera.look_at           = rt::point3f(0, 40, 40);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_filename   = "bunny_campfire.png";
    camera.output_directory  = output_directory;
    camera.render(world, lights);
}

} // namespace scenes

int main(int argc, char** argv)
{
    /*
     * Usage: ./main [scene] [output_directory]
     *   scene             which scene to render, e.g. 7 (defaults to 9)
     *   output_directory  where the image is written (defaults to images/, created if missing)
     */
    int scene                              = argc > 1 ? std::atoi(argv[1]) : 9;
    std::filesystem::path output_directory = argc > 2 ? argv[2] : "images";

    // Scene setup (model loading, BVH builds) and rendering together
    rt::ScopedTimer timer("Total");
    try
    {
        switch (scene)
        {
            case 1:
                scenes::spheres_scene(output_directory);
                break;
            case 2:
                scenes::checkered_spheres(output_directory);
                break;
            case 3:
                scenes::earth(output_directory);
                break;
            case 4:
                scenes::perlin_spheres(output_directory);
                break;
            case 5:
                scenes::quads(output_directory);
                break;
            case 6:
                scenes::simple_light(output_directory);
                break;
            case 7:
                scenes::cornell_box(output_directory);
                break;
            case 8:
                scenes::final_scene(1000, 2000, 40, output_directory);
                break;
            case 9:
                scenes::three_D_model(output_directory);
                break;
            case 10:
                scenes::multiple_models(output_directory);
                break;
            case 11:
                scenes::dragon(output_directory);
                break;
            case 12:
                scenes::sculpture_gallery(output_directory);
                break;
            case 13:
                scenes::neon_stage(output_directory);
                break;
            case 14:
                scenes::bunny_campfire(output_directory);
                break;
            default:
                scenes::final_scene(400, 250, 4, output_directory);
                break;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
