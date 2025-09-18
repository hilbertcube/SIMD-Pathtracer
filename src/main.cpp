#include "rt/ray_tracer.hpp"
#include "rt/mesh.hpp"

void spheres_scene() {
    // World
    rt::hittable_list world;
    
    //auto ground_material = make_shared<rt::lambertian>(rt::color(0.5f, 0.5f, 0.5f));
    auto ground_material = make_shared<rt::checker_texture>(
        0.32f,  // scale
        rt::color(0.5f, 0.5f, 0.5f),    // odd color
        rt::color(0.9f, 0.9f, 0.9f));   // even color
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f, -1000.0f, 0.0f), 1000.0f, make_shared<rt::lambertian>(ground_material)));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            // random material
            float choose_mat = random_float(); // (0, 1)
            rt::point3f center(a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float());

            if ((center - rt::point3f(4.0f, 0.2f, 0.0f)).length() > 0.9f) {
                shared_ptr<rt::material> sphere_material;

                if (choose_mat < 0.8f) {
                    // diffuse
                    auto albedo = rt::color::random() * rt::color::random();
                    sphere_material = make_shared<rt::lambertian>(albedo);
                    auto center2 = center + rt::vec3f(0.0f, random_float(0.0f, 0.5f), 0.0f);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                } else if (choose_mat < 0.95f) {
                    // metal
                    auto albedo = rt::color::random(0.5f, 1.0f);
                    auto fuzz = random_float(0.5f, 1.0f);
                    sphere_material = make_shared<rt::metal>(albedo, fuzz);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<rt::dielectrics>(1.5f);
                    world.add(make_shared<rt::sphere>(center, 0.2f, sphere_material));
                }
            }
        }
    }

    // three big spheres
    auto material1 = make_shared<rt::dielectrics>(1.5f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, 1, 0), 1.0f, material1));

    auto sphere_mat = make_shared<rt::lambertian>(rt::color(0.4f, 0.2f, 0.1f));
    world.add(make_shared<rt::sphere>(rt::point3f(-4, 1, 0), 1.0f, sphere_mat));

    auto material3 = make_shared<rt::metal>(rt::color(0.7f, 0.6f, 0.5f), 0.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(4, 1, 0), 1.0f, material3));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));
    
    rt::Camera cam;
    
    cam.aspect_ratio      = 16.0f/9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = rt::color(0.70f, 0.80f, 1.00f);

    cam.vfov              = 20;
    cam.lookfrom          = rt::point3f(13, 2, 3);
    cam.lookat            = rt::point3f(0, 0, 0);
    cam.vup               = rt::vec3f(0, 1, 0);
    cam.defocus_angle     = 0.6f;
    cam.focus_dist        = 10.0f;
    cam.render_omp(world);
}

void checkered_spheres() {
    // World
    rt::hittable_list world;

    auto checker = make_shared<rt::checker_texture>(0.32f, rt::color(0.2f, 0.3f, 0.1f), rt::color(0.9f, 0.9f, 0.9f));
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f,-10.0f, 0.0f), 10.0f, make_shared<rt::lambertian>(checker)));
    world.add(make_shared<rt::sphere>(rt::point3f(0.0f, 10.0f, 0.0f), 10.0f, make_shared<rt::lambertian>(checker)));

    rt::Camera cam;

    cam.aspect_ratio      = 16.0f / 9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = rt::color(0.70f, 0.80f, 1.00f);

    cam.vfov     = 20;
    cam.lookfrom = rt::point3f(13,2,3);
    cam.lookat   = rt::point3f(0,0,0);
    cam.vup      = rt::vec3f(0,1,0);
    cam.defocus_angle = 0;
    cam.output_filename = "checked_spheres.png";

    cam.render_omp(world);
}

void earth() {
    auto earth_texture = make_shared<rt::image_texture>("texture/earth2048.bmp");
    auto earth_surface = make_shared<rt::lambertian>(earth_texture);
    auto globe = make_shared<rt::sphere>(rt::point3f(0.0f, 0.0f, 0.0f), 2.0f, earth_surface);

    rt::Camera cam;

    cam.aspect_ratio      = 16.0f / 9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = rt::color(0.70f, 0.80f, 1.00f);

    cam.vfov     = 20;
    cam.lookfrom = rt::point3f(0, 0, 12);
    cam.lookat   = rt::point3f(0, 0, 0);
    cam.vup      = rt::vec3f(0, 1, 0);
    cam.defocus_angle = 0.0f;
    
    cam.render_omp(rt::hittable_list(globe));
}

void perlin_spheres() {
    rt::hittable_list world;

    auto pertext = make_shared<rt::noise_texture>(4.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(0, -1000, 0), 1000.0f, make_shared<rt::lambertian>(pertext)));
    world.add(make_shared<rt::sphere>(rt::point3f(0, 2, 0), 2.0f, make_shared<rt::lambertian>(pertext)));

    rt::Camera cam;

    cam.aspect_ratio      = 16.0f / 9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = rt::color(0.70f, 0.80f, 1.00f);
    cam.vfov     = 20;
    cam.lookfrom = rt::point3f(13,2,3);
    cam.lookat   = rt::point3f(0,0,0);
    cam.vup      = rt::vec3f(0,1,0);
    cam.defocus_angle = 0.0f;
    cam.output_filename = "perlin_spheres.png";

    cam.render_omp(world);
}

void quads() {
    rt::hittable_list world;

    // Materials
    auto left_red     = make_shared<rt::lambertian>(rt::color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<rt::lambertian>(rt::color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<rt::lambertian>(rt::color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<rt::lambertian>(rt::color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<rt::lambertian>(rt::color(0.2, 0.8, 0.8));

    // Quads
    world.add(make_shared<rt::quad>(rt::point3f(-3,-2, 5), rt::vec3f(0, 0,-4), rt::vec3f(0, 4, 0), left_red));
    world.add(make_shared<rt::quad>(rt::point3f(-2,-2, 0), rt::vec3f(4, 0, 0), rt::vec3f(0, 4, 0), back_green));
    world.add(make_shared<rt::quad>(rt::point3f( 3,-2, 1), rt::vec3f(0, 0, 4), rt::vec3f(0, 4, 0), right_blue));
    world.add(make_shared<rt::quad>(rt::point3f(-2, 3, 1), rt::vec3f(4, 0, 0), rt::vec3f(0, 0, 4), upper_orange));
    world.add(make_shared<rt::quad>(rt::point3f(-2,-3, 5), rt::vec3f(4, 0, 0), rt::vec3f(0, 0,-4), lower_teal));

    rt::Camera cam;

    cam.aspect_ratio      = 16.0f / 9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 80;
    cam.lookfrom = rt::point3f(0,0,9);
    cam.lookat   = rt::point3f(0,0,0);
    cam.vup      = rt::vec3f(0,1,0);

    cam.defocus_angle = 0;

    cam.render_omp(world);
}

void simple_light() {
    rt::hittable_list world;

    auto pertext = make_shared<rt::noise_texture>(4.0f);
    world.add(make_shared<rt::sphere>(rt::point3f(0,-1000,0), 1000.0f, make_shared<rt::lambertian>(pertext)));
    world.add(make_shared<rt::sphere>(rt::point3f(0,2,0), 2.0f, make_shared<rt::lambertian>(pertext)));

    auto difflight = make_shared<rt::diffuse_light>(rt::color(4,4,4));
    world.add(make_shared<rt::sphere>(rt::point3f(0,7,0), 2.0f, difflight));
    world.add(make_shared<rt::quad>(rt::point3f(3,1,-2), rt::vec3f(2,0,0), rt::vec3f(0,2,0), difflight));

    rt::Camera cam;

    cam.aspect_ratio      = 16.0f / 9.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = rt::color(0,0,0);

    cam.vfov     = 20;
    cam.lookfrom = rt::point3f(26, 3, 6);
    cam.lookat   = rt::point3f(0, 2, 0);
    cam.vup      = rt::vec3f(0, 1, 0);
    cam.defocus_angle = 0;

    cam.render_tiles(world);
}

void cornell_box() {
    rt::hittable_list world;

    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto light = make_shared<rt::diffuse_light>(rt::color(15, 15, 15));

    world.add(make_shared<rt::quad>(rt::point3f(555,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), green));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), red));
    world.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130,0,0), rt::vec3f(0,0,-105), light));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(555,0,0), rt::vec3f(0,0,555), white));
    world.add(make_shared<rt::quad>(rt::point3f(555,555,555), rt::vec3f(-555,0,0), rt::vec3f(0,0,-555), white));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,555), rt::vec3f(555,0,0), rt::vec3f(0,555,0), white));

    // left box
    shared_ptr<rt::hittable> box1 = box(rt::point3f(0,0,0), rt::point3f(165,330,165), white);
    box1 = make_shared<rt::rotate_y>(box1, 15.0f);
    box1 = make_shared<rt::translate>(box1, rt::vec3f(265,0,295));
    
    // right box
    shared_ptr<rt::hittable> box2 = box(rt::point3f(0,0,0), rt::point3f(165,165,165), white);
    box2 = make_shared<rt::rotate_y>(box2, -18.0f);
    box2 = make_shared<rt::translate>(box2, rt::vec3f(130,0,65));

    // sphere
    auto sphere_mat = make_shared<rt::lambertian>(rt::color(0.4f, 0.2f, 0.1f));
    world.add(make_shared<rt::sphere>(rt::point3f(200, 215, 200), 50.0f, sphere_mat));

    // Add pyramid in the center-left area
    auto pyramid_mat = make_shared<rt::lambertian>(rt::color(.12, .45, .5));
    shared_ptr<rt::hittable> pyramid_obj = pyramid(
        rt::point3f(325, 0, 100),    // base min corner
        rt::point3f(475, 0, 200),    // base max corner  
        rt::point3f(400, 150, 150),  // apex point
        pyramid_mat
    );

    world.add(box1);
    world.add(box2);
    world.add(pyramid_obj);

    rt::Camera cam;

    cam.aspect_ratio      = 1.0f;
    cam.image_width       = 600;
    cam.samples_per_pixel = 1000;
    cam.max_depth         = 50;
    cam.background        = rt::color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = rt::point3f(278, 278, -800);
    cam.lookat   = rt::point3f(278, 278, 0);
    cam.vup      = rt::vec3f(0,1,0);
    cam.output_filename = "cornell_box.png";

    cam.defocus_angle = 0;

    cam.render_tiles(world);
}

void final_scene(int image_width, int samples_per_pixel, int max_depth) {
    rt::hittable_list boxes1;
    auto ground = make_shared<rt::lambertian>(rt::color(0.48, 0.83, 0.53));

    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0f;
            auto x0 = -1000.0f + i*w;
            auto z0 = -1000.0f + j*w;
            auto y0 = 0.0f;
            auto x1 = x0 + w;
            auto y1 = random_float(1.0f, 101.0f);
            auto z1 = z0 + w;

            boxes1.add(rt::box(rt::point3f(x0,y0,z0), rt::point3f(x1,y1,z1), ground));
        }
    }

    rt::hittable_list world;

    world.add(make_shared<rt::bvh_node>(boxes1));

    // top light
    auto light = make_shared<rt::diffuse_light>(rt::color(7, 7, 7));
    world.add(make_shared<rt::quad>(rt::point3f(123,554,147), rt::vec3f(300,0,0), rt::vec3f(0,0,265), light));

    // moving sphere
    auto center1 = rt::point3f(400, 400, 200);
    auto center2 = center1 + rt::vec3f(30,0,0);
    auto sphere_material = make_shared<rt::lambertian>(rt::color(0.7, 0.3, 0.1));
    world.add(make_shared<rt::sphere>(center1, center2, 50.0f, sphere_material));

    // dielectrics sphere
    world.add(make_shared<rt::sphere>(rt::point3f(260, 150, 45), 50.0f, make_shared<rt::dielectrics>(1.5f)));

    // metal sphere
    world.add(make_shared<rt::sphere>(
        rt::point3f(0, 150, 145), 50.0f, make_shared<rt::metal>(rt::color(0.8, 0.8, 0.9), 1.0f)
    ));

    auto boundary = make_shared<rt::sphere>(rt::point3f(360,150,145), 70.0f, make_shared<rt::dielectrics>(1.5f));
    world.add(boundary);
    world.add(make_shared<rt::constant_medium>(boundary, 0.2f, rt::color(0.2, 0.4, 0.9)));
    boundary = make_shared<rt::sphere>(rt::point3f(0,0,0), 5000.0f, make_shared<rt::dielectrics>(1.5f));
    world.add(make_shared<rt::constant_medium>(boundary, 0.0001f, rt::color(1,1,1)));

    auto emat = make_shared<rt::lambertian>(make_shared<rt::image_texture>("texture/earth2048.bmp"));
    world.add(make_shared<rt::sphere>(rt::point3f(400,200,400), 100.0f, emat));
    auto pertext = make_shared<rt::noise_texture>(0.2f);
    world.add(make_shared<rt::sphere>(rt::point3f(220,280,300), 80.0f, make_shared<rt::lambertian>(pertext)));

    rt::hittable_list boxes2;
    auto white = make_shared<rt::lambertian>(rt::color(0.73, 0.73, 0.73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<rt::sphere>(rt::point3f::random(0,165), 10.0f, white));
    }

    world.add(make_shared<rt::translate>(
        make_shared<rt::rotate_y>(make_shared<rt::bvh_node>(boxes2), 15.0f), rt::vec3f(-100, 270, 395)
        )
    );

    rt::Camera cam;

    cam.aspect_ratio      = 1.0f;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = rt::color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = rt::point3f(478, 278, -600);
    cam.lookat   = rt::point3f(278, 278, 0);
    cam.vup      = rt::vec3f(0,1,0);

    cam.defocus_angle = 0;
    cam.output_filename = "final_scene.png";

    cam.render_tiles(world);
}

void three_D_model() {
    rt::hittable_list world;

    // Cornell box
    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto light = make_shared<rt::diffuse_light>(rt::color(15, 15, 15));

    auto blue = make_shared<rt::lambertian>(rt::color(0.4, 0.6, 0.9));

    world.add(make_shared<rt::quad>(rt::point3f(555,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), green));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), red));
    world.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130,0,0), rt::vec3f(0,0,-105), light));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(555,0,0), rt::vec3f(0,0,555), white));
    world.add(make_shared<rt::quad>(rt::point3f(555,555,555), rt::vec3f(-555,0,0), rt::vec3f(0,0,-555), white));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,555), rt::vec3f(555,0,0), rt::vec3f(0,555,0), blue));

    // // Mirror
    // auto mirror = make_shared<rt::metal>(rt::color(0.95, 0.95, 0.95), 0.0f);

    // // Right wall mirror (x = 555)
    // {
    //     rt::point3f origin(555 - 0.01f, 50, 50);
    //     rt::vec3f height(0, 455, 0);
    //     rt::vec3f width(0, 0, 455);
    //     world.add(make_shared<rt::quad>(origin, height, width, mirror));
    // }
    // // Left wall mirror (x = 0)
    // {
    //     rt::point3f origin(0 + 0.01f, 50, 50);   // tiny offset into +x to avoid z-fighting
    //     rt::vec3f height(0, 455, 0);
    //     rt::vec3f width(0, 0, 455);
    //     world.add(make_shared<rt::quad>(origin, height, width, mirror));
    // }
    // // Back wall mirror (z = 555)
    // {
    //     rt::point3f origin(50, 50, 555 - 0.01f);
    //     rt::vec3f height(455, 0, 0);
    //     rt::vec3f width(0, 455, 0);
    //     world.add(make_shared<rt::quad>(origin, height, width, mirror));
    // }

    // // Dragon model
    // auto dragon_mat = make_shared<rt::lambertian>(rt::color(0.9, 0.8, 0));
    // auto dragon_obj = rt::load_obj("model/dragon.obj", dragon_mat);
    // rt::transform_mesh(*dragon_obj, 2.4f, rt::vec3f(278,100,210));
    // auto dragon_bvh = make_shared<rt::bvh_node>(dragon_obj->objects, 0, dragon_obj->objects.size());
    // world.add(dragon_bvh);

    // Tea pot
    auto tea_mat = make_shared<rt::metal>(rt::color(0, 0, 0.8), 0.0f);
    auto tea_pot = rt::load_obj("model/teapot.obj", tea_mat);
    rt::transform_mesh(*tea_pot, 80.0f,rt::vec3f(278, 0, 278));
    auto teapot_bvh = make_shared<rt::bvh_node>(tea_pot->objects, 0, tea_pot->objects.size());
    world.add(teapot_bvh);

    rt::Camera cam;
    cam.aspect_ratio      = 1.0f;
    cam.image_width       = 1000;
    cam.samples_per_pixel = 2000;
    cam.max_depth         = 50;
    cam.background        = rt::color(0,0,0);
    cam.vfov     = 40;
    cam.lookfrom = rt::point3f(278, 278, -800);
    cam.lookat   = rt::point3f(278, 278, 0);
    cam.vup      = rt::vec3f(0,1,0);
    cam.output_filename = "3d_model.png";
    cam.defocus_angle = 0;
    cam.render_tiles(world);
}

void multiple_models() {
    rt::hittable_list world;

    // Cornell box
    auto red   = make_shared<rt::lambertian>(rt::color(.65, .05, .05));
    auto white = make_shared<rt::lambertian>(rt::color(.73, .73, .73));
    auto green = make_shared<rt::lambertian>(rt::color(.12, .45, .15));
    auto light = make_shared<rt::diffuse_light>(rt::color(15, 15, 15));

    world.add(make_shared<rt::quad>(rt::point3f(555,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), green));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(0,555,0), rt::vec3f(0,0,555), red));
    world.add(make_shared<rt::quad>(rt::point3f(343, 554, 332), rt::vec3f(-130,0,0), rt::vec3f(0,0,-105), light));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,0), rt::vec3f(555,0,0), rt::vec3f(0,0,555), white));
    world.add(make_shared<rt::quad>(rt::point3f(555,555,555), rt::vec3f(-555,0,0), rt::vec3f(0,0,-555), white));
    world.add(make_shared<rt::quad>(rt::point3f(0,0,555), rt::vec3f(555,0,0), rt::vec3f(0,555,0), white));

    // left box
    shared_ptr<rt::hittable> box1 = box(rt::point3f(0,0,0), rt::point3f(165,165,165), white);
    box1 = make_shared<rt::rotate_y>(box1, 15.0f);
    box1 = make_shared<rt::translate>(box1, rt::vec3f(285,0,295));
    world.add(box1);

    // right box
    shared_ptr<rt::hittable> box2 = box(rt::point3f(0,0,0), rt::point3f(165,165,165), white);
    box2 = make_shared<rt::rotate_y>(box2, -18.0f);
    box2 = make_shared<rt::translate>(box2, rt::vec3f(130,0,65));
    world.add(box2);

    // Suzanne model
    auto dragon_mat = make_shared<rt::lambertian>(rt::color(0.9, 0.8, 0));
    auto dragon_obj = rt::load_obj("model/suzanne.obj", dragon_mat);
    rt::transform_mesh(*dragon_obj, 80.0f, rt::vec3f(110, 165, -450));
    auto dragon_bvh = make_shared<rt::bvh_node>(dragon_obj->objects, 0, dragon_obj->objects.size());
    auto rotated = make_shared<rt::rotate_y>(dragon_bvh, 200);
    auto suzanne_final = make_shared<rt::translate>(rotated, rt::vec3f(278, 0, 278));
    world.add(suzanne_final);

    // Tea pot
    auto tea_mat = make_shared<rt::lambertian>(rt::color(0.8, 0.8, 0.8));
    auto tea_pot = rt::load_obj("model/teapot.obj", tea_mat);
    rt::transform_mesh(*tea_pot, 40.0f,rt::vec3f(185, 160, 220));
    auto teapot_bvh = make_shared<rt::bvh_node>(tea_pot->objects, 0, tea_pot->objects.size());
    world.add(teapot_bvh);

    // Spot the cow
    auto spot_mat = make_shared<rt::lambertian>(rt::color(0, 0.8, 0.9));
    auto spot_obj = rt::load_obj("model/spot.obj", spot_mat);
    rt::transform_mesh(*spot_obj, 90.0f, rt::vec3f(420, 60, 80));
    auto spot_bvh = make_shared<rt::bvh_node>(spot_obj->objects, 0, spot_obj->objects.size());
    auto spot_rotated = make_shared<rt::rotate_y>(spot_bvh, 45.0f);
    auto spot_final = make_shared<rt::translate>(spot_rotated, rt::vec3f(65, 0, 290));
    world.add(spot_final);

    rt::Camera cam;
    cam.aspect_ratio      = 1.0f;
    cam.image_width       = 600;
    cam.samples_per_pixel = 1000;
    cam.max_depth         = 50;
    cam.background        = rt::color(0,0,0);
    cam.vfov     = 40;
    cam.lookfrom = rt::point3f(278, 278, -800);
    cam.lookat   = rt::point3f(278, 278, 0);
    cam.vup      = rt::vec3f(0,1,0);
    cam.output_filename = "multiple_3d_models.png";
    cam.defocus_angle = 0;
    cam.render_tiles(world);
}

int main() {
    rt::benchmark::Timer timer("Rendering process");
    timer.showMilli().showSeconds().showMinutes();
    switch (9) {
        case 1:  spheres_scene();               break;
        case 2:  checkered_spheres();           break;
        case 3:  earth();                       break;
        case 4:  perlin_spheres();              break;
        case 5:  quads();                       break;
        case 6:  simple_light();                break;
        case 7:  cornell_box();                 break;
        case 8:  final_scene(1000, 2000, 40);   break;
        case 9: three_D_model();                break;
        case 10: multiple_models();             break;
        default: final_scene(400,   250,  4);   break;
    }
    return 0;
}

// cmake --build .\build\ --config Debug; .\build\Debug\main
// cmake --build .\build\ --config Release; .\build\Release\main

// cornell box [Debug]
// omp: 168.956217 s
// tiles + omp: 160.066773 s, 164.114298 s, 163.436055 s
// SIMD + omp + tiles: 141.309591 s, 150.529486 s, 142.902596 s, 136.291196 s
