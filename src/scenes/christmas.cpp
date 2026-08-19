#include "scenes/scenes.h"

#include <cmath>
#include <string>

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

namespace
{

// Give the triangles of a textured mesh whose texture color is bright orange a glowing material.
// A model that paints its lamp glass with a palette color gets a real light this way.
void make_orange_glow(rt::hittable_list& mesh, const rt::texture& colors,
                      shared_ptr<rt::material> glow)
{
    for (const auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<rt::attributed_triangle>(object);
        if (!face)
        {
            continue;
        }
        float u         = (face->texture_u[0] + face->texture_u[1] + face->texture_u[2]) / 3.0f;
        float v         = (face->texture_v[0] + face->texture_v[1] + face->texture_v[2]) / 3.0f;
        rt::color color = colors.value(u, v, rt::point3f(0, 0, 0));
        if (color.r() > 0.9f && color.g() > 0.55f && color.b() < 0.35f)
        {
            face->surface_material = glow;
        }
    }
}

} // namespace

void christmas(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights; // what the shadow rays aim at: geometry only, materials are ignored

    const std::string models = "models/christmas/";
    auto colormap            = make_shared<rt::image_texture>("models/christmas/colormap.png");
    auto painted             = make_shared<rt::lambertian>(colormap); // the Kenney models' colors

    // Place one of the Kenney models, standing `height` tall at `base`, turned by `turn` degrees
    // (0 faces the camera)
    auto place = [&](const char* name, float height, const rt::point3f& base, float turn)
    {
        world.add(load_textured_model(models + name + ".obj", painted, height, base, turn));
    };

    // ---------------------------------------------------------------- ground, moon
    auto snow = make_shared<rt::lambertian>(rt::color(0.88, 0.92, 1.0));
    world.add(make_shared<rt::quad>(rt::point3f(-4000, 0, -2000), rt::vec3f(8000, 0, 0),
                                    rt::vec3f(0, 0, 6000), snow));

    // The moonlight comes from above and behind the camera, so the snow is lit and the shadows
    // fall away from the viewer. Shadow rays aim at it four times as often as at a bulb.
    const rt::point3f moonlight_center(-700, 1000, -1000);
    world.add(make_shared<rt::sphere>(moonlight_center, 120,
                                      make_shared<rt::diffuse_light>(rt::color(40, 45, 60))));
    for (int i = 0; i < 4; ++i)
    {
        lights.add(make_shared<rt::sphere>(moonlight_center, 120, shared_ptr<rt::material>()));
    }

    // The moon in the sky. It is dim, since it only needs to look white, and no shadow rays aim
    // at it, so it adds no noise.
    world.add(make_shared<rt::sphere>(rt::point3f(640, 740, 1650), 85,
                                      make_shared<rt::diffuse_light>(rt::color(3.5, 3.8, 4.5))));

    // ---------------------------------------------------------------- the tree
    const float tree_height = 300;
    place("tree-decorated-snow", tree_height, rt::point3f(0, 0, 0), 0.0f);

    // String lights spiral up the tree. Their radius follows the tree's outline, which tapers
    // roughly linearly with height.
    const rt::color bulb_colors[4] = {rt::color(7, 4.2, 1.4), rt::color(7, 0.7, 0.7),
                                      rt::color(0.8, 5.5, 1.8), rt::color(1.2, 2.2, 8)};
    const int bulb_count           = 46;
    for (int i = 0; i < bulb_count; ++i)
    {
        float fraction = 0.14f + 0.72f * i / (bulb_count - 1);
        float radius   = tree_height * (0.33f * (1.0f - fraction) + 0.012f);
        float angle    = 2.4f * i;
        rt::point3f position(radius * std::cos(angle), tree_height * fraction,
                             radius * std::sin(angle));
        auto bulb = make_shared<rt::diffuse_light>(bulb_colors[i % 4]);
        world.add(make_shared<rt::sphere>(position, 3.0f, bulb));
        if (i % 4 == 0)
        {
            lights.add(make_shared<rt::sphere>(position, 3.0f, shared_ptr<rt::material>()));
        }
    }

    // ---------------------------------------------------------------- presents under the tree
    place("present-a-cube", 56, rt::point3f(-95, 0, -70), 15.0f);
    place("present-b-cube", 46, rt::point3f(70, 0, -85), -20.0f);
    place("present-a-round", 38, rt::point3f(-45, 0, -118), 35.0f);
    place("present-b-round", 62, rt::point3f(118, 0, -30), 5.0f);
    place("present-b-cube", 30, rt::point3f(28, 0, -128), 40.0f);
    place("present-a-cube", 72, rt::point3f(-135, 0, 15), -10.0f);

    // A gold-foil present with a red ribbon and bow, made from boxes and spheres
    {
        auto foil        = make_shared<rt::metal>(rt::color(0.95, 0.72, 0.30), 0.04f);
        auto ribbon      = make_shared<rt::lambertian>(rt::color(0.75, 0.05, 0.07));
        const float size = 44, half = size / 2, strip = 4;
        const rt::point3f c(-10, 0, -72);
        world.add(rt::box(rt::point3f(c.x() - half, 0, c.z() - half),
                          rt::point3f(c.x() + half, size, c.z() + half), foil));
        world.add(rt::box(rt::point3f(c.x() - strip, 0, c.z() - half - 0.5f),
                          rt::point3f(c.x() + strip, size + 0.5f, c.z() + half + 0.5f), ribbon));
        world.add(rt::box(rt::point3f(c.x() - half - 0.5f, 0, c.z() - strip),
                          rt::point3f(c.x() + half + 0.5f, size + 0.5f, c.z() + strip), ribbon));
        world.add(make_shared<rt::sphere>(rt::point3f(c.x() - 7, size + 5, c.z()), 7.5f, ribbon));
        world.add(make_shared<rt::sphere>(rt::point3f(c.x() + 7, size + 5, c.z()), 7.5f, ribbon));
    }

    // Chrome baubles that fell into the snow, reflecting the string lights
    const struct
    {
        rt::point3f center;
        float radius;
        rt::color color;
    } baubles[4] = {{rt::point3f(-58, 10, -165), 10, rt::color(0.88, 0.07, 0.07)},
                    {rt::point3f(62, 8, -158), 8, rt::color(0.95, 0.72, 0.30)},
                    {rt::point3f(-150, 9, -120), 9, rt::color(0.90, 0.90, 0.94)},
                    {rt::point3f(150, 8, -95), 8, rt::color(0.10, 0.25, 0.85)}};

    for (const auto& bauble : baubles)
    {
        world.add(make_shared<rt::sphere>(bauble.center, bauble.radius,
                                          make_shared<rt::metal>(bauble.color, 0.01f)));
    }

    // ---------------------------------------------------------------- characters and props
    place("snowman-hat", 125, rt::point3f(215, 0, -85), 40.0f);
    place("nutcracker", 105, rt::point3f(-120, 0, 70), -15.0f);
    place("gingerbread-man", 42, rt::point3f(48, 0, -178), 10.0f);
    place("gingerbread-woman", 42, rt::point3f(82, 0, -172), -15.0f);
    place("candy-cane-red", 62, rt::point3f(165, 0, -35), 20.0f);
    place("candy-cane-green", 58, rt::point3f(-160, 0, -45), -25.0f);
    place("sled", 42, rt::point3f(-235, 0, 25), -30.0f);

    // Rudolph: a reindeer whose nose is a red light, found as its frontmost vertex
    {
        const float turn = -35.0f;
        auto mesh        = rt::load_obj_with_attributes(models + "reindeer.obj", painted);
        world.add(place_model(*mesh, 125, rt::point3f(-185, 0, -85), turn, 0.0f));
        const float radians = rt::degrees_to_radians(turn);
        const rt::vec3f facing(-std::sin(radians), 0.0f, -std::cos(radians));
        rt::point3f nose = frontmost_vertex(*mesh, facing) + 2.5f * facing;
        auto red_glow    = make_shared<rt::diffuse_light>(rt::color(7, 0.2, 0.2));
        world.add(make_shared<rt::sphere>(nose, 3.6f, red_glow));
        lights.add(make_shared<rt::sphere>(nose, 3.6f, shared_ptr<rt::material>()));
    }

    // Two lamp posts whose glass glows. The glass is part of the model, so its triangles become
    // emitters; a light inside it gives the shadow rays something to aim at.
    for (float x : {320.0f, -320.0f})
    {
        const float height = 190;
        const rt::point3f base(x, 0, 120);
        auto mesh = rt::load_obj_with_attributes(models + "lantern.obj", painted);
        make_orange_glow(*mesh, *colormap, make_shared<rt::diffuse_light>(rt::color(16, 11, 4)));
        world.add(place_model(*mesh, height, base, 0.0f, 0.0f));
        const rt::point3f glass_center(x, 0.815f * height, 120);
        for (int i = 0; i < 2; ++i)
        {
            lights.add(make_shared<rt::sphere>(glass_center, 6.0f, shared_ptr<rt::material>()));
        }

        // A small ball of mist around the glass, so the lamp gets a soft halo
        world.add(make_shared<rt::constant_medium>(
            make_shared<rt::sphere>(glass_center, 34.0f, shared_ptr<rt::material>()), 0.008f,
            rt::color(1.0, 0.85, 0.6)));
    }

    // ---------------------------------------------------------------- the woods behind
    const struct
    {
        const char* name;
        float x, z, height;
    } woods[9] = {{"tree-snow-a", -330, 190, 300}, {"tree", -190, 330, 360},
                  {"tree-snow-c", -40, 420, 330},  {"tree-snow-b", 120, 300, 290},
                  {"tree-snow-a", 300, 210, 330},  {"tree", 430, 360, 380},
                  {"tree-snow-b", -470, 380, 400}, {"tree-snow-c", 560, 140, 280},
                  {"tree", -540, 120, 300}};

    for (const auto& tree : woods)
    {
        place(tree.name, tree.height, rt::point3f(tree.x, 0, tree.z), 0.0f);
    }
    place("rocks-medium", 70, rt::point3f(-380, 0, 90), 20.0f);
    place("snow-pile", 34, rt::point3f(-250, 0, 120), 0.0f);
    place("snow-pile", 30, rt::point3f(225, 0, 150), 90.0f);
    place("snow-pile", 28, rt::point3f(330, 0, -60), 30.0f);

    // ---------------------------------------------------------------- falling snow
    auto flake = make_shared<rt::lambertian>(rt::color(0.95, 0.95, 1.0));
    for (int i = 0; i < 650; ++i)
    {
        rt::point3f position(rt::random_float(-460, 460), rt::random_float(6, 420),
                             rt::random_float(-300, 420));
        world.add(make_shared<rt::sphere>(position, rt::random_float(1.0f, 2.6f), flake));
    }

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.012, 0.020, 0.050); // night sky
    camera.field_of_view     = 40;
    camera.look_from         = rt::point3f(45, 85, -560);
    camera.look_at           = rt::point3f(0, 135, 0);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0.45f; // shallow depth of field: far lights melt into bokeh
    camera.focus_distance    = 500;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
