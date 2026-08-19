#include "scenes/scenes.h"

#include <cmath>

#include "rt/materials/advanced_materials.h"
#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

void material_showcase(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights; // what the shadow rays aim at: geometry only, materials are ignored

    // ---------------------------------------------------------------- the stage
    // A glossy dark floor, so the teapots are reflected in it
    world.add(make_shared<rt::quad>(rt::point3f(-2000, 0, -1400), rt::vec3f(4000, 0, 0),
                                    rt::vec3f(0, 0, 2800),
                                    make_shared<rt::coated>(rt::color(0.04, 0.04, 0.05))));

    // A checkered backdrop, for the glass to refract
    auto checks = make_shared<rt::lambertian>(make_shared<rt::checker_texture>(
        90.0f, rt::color(0.50, 0.50, 0.53), rt::color(0.10, 0.10, 0.12)));
    world.add(make_shared<rt::quad>(rt::point3f(-2000, 0, 780), rt::vec3f(4000, 0, 0),
                                    rt::vec3f(0, 1000, 0), checks));

    // Three steps going up toward the backdrop, one row of teapots on each (the front row stands
    // on the floor)
    const float step_height = 45.0f;
    auto stone              = make_shared<rt::coated>(rt::color(0.55, 0.55, 0.58));
    world.add(rt::box(rt::point3f(-640, 0, 120), rt::point3f(640, step_height, 300), stone));
    world.add(rt::box(rt::point3f(-640, 0, 300), rt::point3f(640, 2 * step_height, 480), stone));
    world.add(rt::box(rt::point3f(-640, 0, 480), rt::point3f(640, 3 * step_height, 660), stone));

    // Lights: a big softbox overhead, strips at the sides for long reflections on the metals
    // (out of the picture), and a dim fill behind the camera. Each is also a target for the
    // shadow rays.
    auto add_light = [&](const rt::point3f& corner, const rt::vec3f& edge_u,
                         const rt::vec3f& edge_v, const rt::color& emission, int weight)
    {
        world.add(make_shared<rt::quad>(corner, edge_u, edge_v,
                                        make_shared<rt::diffuse_light>(emission)));
        for (int i = 0; i < weight; ++i)
        {
            lights.add(make_shared<rt::quad>(corner, edge_u, edge_v, shared_ptr<rt::material>()));
        }
    };
    add_light(rt::point3f(-600, 700, -300), rt::vec3f(1200, 0, 0), rt::vec3f(0, 0, 1000),
              rt::color(2.4f, 2.4f, 2.4f), 3);
    add_light(rt::point3f(-820, 20, -250), rt::vec3f(0, 420, 0), rt::vec3f(0, 0, 400),
              rt::color(1.5f, 1.5f, 1.7f), 1);
    add_light(rt::point3f(820, 20, -250), rt::vec3f(0, 0, 400), rt::vec3f(0, 420, 0),
              rt::color(1.5f, 1.5f, 1.7f), 1);
    add_light(rt::point3f(-600, 100, -1150), rt::vec3f(1200, 0, 0), rt::vec3f(0, 400, 0),
              rt::color(0.6f, 0.6f, 0.6f), 1);

    // ---------------------------------------------------------------- the teapots
    const float height  = 90.0f;
    const float turn    = 20.0f;
    const float radians = rt::degrees_to_radians(turn);
    const rt::vec3f spout(std::cos(radians), 0.0f, -std::sin(radians)); // where the spout points

    // Slot i of a row of five, counted from the left of the picture (the camera's right is -x)
    const float spacing = 210.0f;
    auto front          = [&](int i)
    {
        return rt::point3f(2 * spacing - spacing * i, 0, -60);
    };
    auto middle = [&](int i)
    {
        return rt::point3f(2 * spacing - spacing * i, step_height, 210);
    };
    auto top = [&](int i)
    {
        return rt::point3f(2 * spacing - spacing * i, 2 * step_height, 390);
    };
    auto peak = [&](int i)
    {
        return rt::point3f(2 * spacing - spacing * i, 3 * step_height, 570);
    };

    // A smooth-shaded teapot in a slot. If asked, it also reports where its spout ends.
    auto teapot = [&](shared_ptr<rt::material> material, const rt::point3f& base,
                      rt::point3f* spout_tip = nullptr)
    {
        auto mesh = rt::load_obj_smooth("models/teapot.obj", material);
        auto body = place_model(*mesh, height, base, turn, 0.0f);
        if (spout_tip)
        {
            *spout_tip = frontmost_vertex(*mesh, spout);
        }
        return body;
    };

    // A plume of small overlapping puffs of mist that scatters mostly forward, so it glows
    // against the light behind it. It starts narrow at the end of the spout, then the puffs
    // drift and widen as they rise, and thin out.
    auto add_steam = [&](const rt::point3f& tip)
    {
        auto steam           = make_shared<rt::mist>(rt::color(0.95, 0.95, 1.0), 0.6f);
        const int puff_count = 22;
        for (int i = 0; i < puff_count; ++i)
        {
            const float rise  = i / static_cast<float>(puff_count - 1);
            const float drift = 2.0f + 26.0f * rise;
            const rt::point3f center =
                tip + rt::vec3f(rt::random_float(-drift, drift), 3.0f + 125.0f * rise,
                                rt::random_float(-drift, drift));
            const float radius  = 5.0f + 16.0f * rise + rt::random_float(0.0f, 3.0f);
            const float density = 0.22f * (1.0f - 0.92f * rise);
            world.add(make_shared<rt::constant_medium>(
                make_shared<rt::sphere>(center, radius, shared_ptr<rt::material>()), density,
                steam));
        }
    };

    // Front row: the original materials, and a coated plastic ------------------------------
    // 1. Matte diffuse, with a 3D checker pattern
    world.add(teapot(make_shared<rt::lambertian>(make_shared<rt::checker_texture>(
                         14.0f, rt::color(0.90, 0.85, 0.70), rt::color(0.08, 0.30, 0.35))),
                     front(0)));
    // 2. Fuzzy metal: a mirror blurred by random perturbation
    world.add(teapot(make_shared<rt::metal>(rt::color(0.70, 0.75, 0.80), 0.3f), front(1)));
    // 3. Clear glass
    world.add(teapot(make_shared<rt::dielectrics>(1.5f), front(2)));
    // 4. A lamp: an emissive teapot
    world.add(teapot(make_shared<rt::diffuse_light>(rt::color(1.0, 0.85, 0.55)), front(3)));
    // 5. Coated plastic
    world.add(teapot(make_shared<rt::coated>(rt::color(0.75, 0.04, 0.05)), front(4)));

    // Middle row: glass and metals ---------------------------------------------------------
    // 6. Emerald glass that darkens where it is thick
    world.add(teapot(make_shared<rt::tinted_dielectric>(1.5f, rt::color(0.15, 0.75, 0.35), 35.0f),
                     middle(0)));
    // 7. Rough copper (GGX)
    world.add(teapot(make_shared<rt::conductor>(rt::color(0.95, 0.64, 0.54), 0.35f), middle(1)));
    // 8. Gold mirror, with Fresnel
    world.add(teapot(make_shared<rt::conductor>(rt::color(1.0, 0.78, 0.34), 0.0f), middle(2)));
    // 9. Hammered silver: rough metal with small dents from bump mapping
    world.add(
        teapot(make_shared<rt::bumped>(
                   make_shared<rt::conductor>(rt::color(0.95, 0.95, 0.97), 0.18f), 0.35f, 0.9f),
               middle(3)));
    // 10. Iridescent pearl
    world.add(teapot(make_shared<rt::iridescent>(rt::color(0.85, 0.85, 0.88), 520.0f), middle(4)));

    // Third row: the unusual ones, and the original Perlin noise ----------------------------
    // 11. Crystal that splits light into colors
    world.add(teapot(make_shared<rt::dispersive_dielectric>(1.45f, 1.52f, 1.62f), top(0)));
    // 12. Velvet
    world.add(teapot(make_shared<rt::velvet>(rt::color(0.30, 0.02, 0.08),
                                             rt::color(1.0, 0.55, 0.65), 3.0f, 1.0f),
                     top(1)));
    // 13. Perlin noise: the original texture of the book's marble spheres, with its noise
    // scaled up to the size of a teapot
    world.add(
        teapot(make_shared<rt::lambertian>(make_shared<rt::noise_texture>(0.18f, 0.045f)), top(2)));
    // 14. Frosted glass: glass with a rough surface
    world.add(teapot(make_shared<rt::rough_dielectric>(1.5f, 0.45f), top(3)));
    // 15. Brushed steel: grooves running around the teapot, so highlights streak up and down it
    world.add(
        teapot(make_shared<rt::brushed_metal>(rt::color(0.80, 0.82, 0.85), 0.12f, 0.55f), top(4)));

    // Top row: clay, and the two that steam --------------------------------------------------
    // 16. Terracotta clay: rough diffuse (Oren-Nayar)
    world.add(teapot(make_shared<rt::oren_nayar>(rt::color(0.80, 0.42, 0.26), 0.8f), peak(1)));
    // 17. Jade: a smooth glass skin around a scattering medium, which is how light behaves in
    // translucent stone (subsurface scattering)
    {
        rt::point3f tip;
        auto jade = teapot(make_shared<rt::dielectrics>(1.5f), peak(2), &tip);
        world.add(jade);
        world.add(make_shared<rt::constant_medium>(jade, 0.10f, rt::color(0.35, 0.78, 0.50)));
        add_steam(tip);
    }
    // 18. Lava: glass filled with a glowing medium
    {
        rt::point3f tip;
        auto lava = teapot(make_shared<rt::dielectrics>(1.5f), peak(3), &tip);
        world.add(lava);
        world.add(make_shared<rt::constant_medium>(
            lava, 0.04f,
            make_shared<rt::glowing_medium>(rt::color(1.0, 0.45, 0.08),
                                            rt::color(1.8, 0.45, 0.04))));
        add_steam(tip);
    }

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 2000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.02, 0.02, 0.03);
    camera.field_of_view     = 32;
    camera.look_from         = rt::point3f(0, 340, -1100);
    camera.look_at           = rt::point3f(0, 105, 230);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
