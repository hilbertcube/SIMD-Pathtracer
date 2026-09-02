#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// A ring of colorful bunnies around a glowing orb at night, with a horse watching from the dark
void bunny_campfire(const std::filesystem::path& output_path)
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
        world.add(load_model("models/stanford-bunny.obj", make_shared<rt::lambertian>(colors[i]),
                             70, base, facing_degrees));
    }

    auto horse_coat = make_shared<rt::lambertian>(rt::color(0.45, 0.30, 0.18));
    world.add(
        load_model("models/horse.obj", horse_coat, 190, rt::point3f(-260, 0, 330), -30.0f, -90.0f));

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
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
