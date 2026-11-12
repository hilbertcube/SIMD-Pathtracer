#include "scenes/scenes.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"
#include "scenes/load_obj_by_material.h"

// Three Art Deco towers in a wooded park under a pink sky. About 2 m to the unit: the tallest
// tower is 115 units, the trees 10 to 14.

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

namespace
{

// Where things are, shared by the parts of the scene
struct layout
{
    float plaza_x = 85, plaza_z = 35; // half the plaza's size
    float plaza_height = 0.2f;
    rt::point3f path_start{0, 0.1f, plaza_z}; // middle of the plaza's front edge
    rt::vec3f path_direction{0, 0, 1}, path_across{-1, 0, 0};
    float path_width = 8, path_length = 400;
    rt::point3f look_from{80, 48, 235}; // right of the path and raised, to see the towers' sides
    rt::point3f look_at{-5, 55, 0};
};

// A fixed pseudo-random number in [0, 1), so the park is the same on every run
float hash(int a, int b, int c)
{
    uint32_t h = static_cast<uint32_t>(a) * 73856093u ^ static_cast<uint32_t>(b) * 19349663u ^
                 static_cast<uint32_t>(c) * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return (h & 0xffffff) / float(0x1000000);
}

// The camera's view direction in the ground plane
rt::vec3f flat_view(const layout& site)
{
    const rt::vec3f view = site.look_at - site.look_from;
    return unit_vector(rt::vec3f(view.x(), 0, view.z()));
}

// One material from the model's PBR textures (4k copies of its 8k originals)
shared_ptr<rt::material> tower_material(const std::string& code)
{
    auto map = [&](const char* kind)
    {
        const std::string file = "models/Building/textures/4k/MAT " + code + "_" + kind + ".jpg";
        return make_shared<rt::image_texture>(file.c_str());
    };
    return make_shared<rt::metallic_roughness>(map("Base_color"), map("Metallic"),
                                               map("Roughness"));
}

// The three towers, each from its own file, rearranged with the tallest (C) in the middle
void add_towers(rt::hittable_list& world, const layout& site)
{
    auto stone_and_bronze = tower_material("FC01");
    auto podium           = tower_material("FC08");
    auto pick             = [&](const std::string& name)
    {
        return name == "MAT_FC08" ? podium : stone_and_bronze;
    };

    struct placement
    {
        const char* file;
        float model_x, x; // where it stands in the original model, and here
    };

    const placement placements[] = {
        {"tower_c", -53.25f, 0.0f}, {"tower_b", 0.0f, -50.0f}, {"tower_a", 47.3f, 50.0f}};
    for (const auto& tower : placements)
    {
        auto mesh =
            load_obj_by_material(std::string("models/Building/OBJ/") + tower.file + ".obj", pick);
        world.add(make_shared<rt::translate>(
            make_shared<rt::bvh_node>(mesh->objects, 0, mesh->objects.size()),
            rt::vec3f(tower.x - tower.model_x, site.plaza_height, 0)));
    }
}

// A lawn (texture repeating every 7.5 units), a stone plaza and a path out from it
void add_ground(rt::hittable_list& world, const layout& site)
{
    const float size = 14000;
    auto grass       = make_shared<rt::tiled_texture>(
        make_shared<rt::image_texture>("texture/aerial_grass_rock_diff_2k.jpg"), size / 7.5f,
        size / 7.5f);
    world.add(make_shared<rt::quad>(rt::point3f(-size / 2, 0, -size / 2), rt::vec3f(size, 0, 0),
                                    rt::vec3f(0, 0, size), make_shared<rt::lambertian>(grass)));

    auto stone = make_shared<rt::lambertian>(rt::color(0.30, 0.29, 0.27));
    world.add(rt::box(rt::point3f(-site.plaza_x, 0, -site.plaza_z),
                      rt::point3f(site.plaza_x, site.plaza_height, site.plaza_z), stone));
    world.add(make_shared<rt::quad>(site.path_start - (site.path_width / 2) * site.path_across,
                                    site.path_length * site.path_direction,
                                    site.path_width * site.path_across, stone));
}

/*
 * The five trees in models/Trees/OBJ, each standing at its own height with its base at the
 * origin. Leaves, twigs and blossoms are cut-out cards (see cutout). The pack has no cherry bark
 * texture, so the cherry uses Bark.png.
 */
std::vector<shared_ptr<rt::hittable>> load_tree_kinds()
{
    const std::string folder = "models/Trees/texture/";
    auto image               = [&](const std::string& file)
    {
        return make_shared<rt::image_texture>((folder + file).c_str());
    };
    auto card = [&](const std::string& color_file,
                    const std::string& mask_file) -> shared_ptr<rt::material>
    {
        return make_shared<rt::cutout>(make_shared<rt::lambertian>(image(color_file)),
                                       folder + mask_file);
    };
    auto plain = [](const rt::color& albedo) -> shared_ptr<rt::material>
    {
        return make_shared<rt::lambertian>(albedo);
    };

    const std::string oak = "oak_leaf.png";
    const std::string mango =
        "pngtree-mango-leaves-isolated-bright-png-image_11272939-removebg-preview.png";
    auto pale_bark   = make_shared<rt::lambertian>(image("tree_bark_03_diff_2k.png"));
    auto twig_leaves = card("Leaf.png", "Leaf.png");
    auto twigs       = card("Branch.png", "Branch.png");

    // The pack has no .mtl, so its material names are mapped by hand
    const std::unordered_map<std::string, shared_ptr<rt::material>> materials = {
        {"Leaf", twig_leaves},
        {"Leaf.001", twig_leaves},
        {"Branch", twigs},
        {"Branch.001", twigs},
        {"Material.001", card(oak, oak)},
        {"Material.006", card(mango, mango)},
        {"Material.002", pale_bark},
        {"Material.005", pale_bark},
        {"v176CherryFlower.002", card("v176CherryFlower_BC.jpg", "v176CherryFlower_O.jpg")},
        {"v176CherryFlowerStalk.001", plain(rt::color(0.25, 0.16, 0.10))},
        {"v176CherryBud.001", plain(rt::color(0.85, 0.45, 0.55))},
    };
    shared_ptr<rt::material> bark = make_shared<rt::lambertian>(image("Bark.png"));
    auto pick                     = [&](const std::string& name)
    {
        auto found = materials.find(name);
        return found == materials.end() ? bark : found->second; // bark, knots, and the default
    };

    const char* files[]   = {"tree_a", "tree_b", "tree_c", "tree_d", "cherry"};
    const float heights[] = {12.0f, 11.0f, 14.0f, 10.5f, 10.0f};
    std::vector<shared_ptr<rt::hittable>> kinds;
    for (int k = 0; k < 5; ++k)
    {
        auto mesh =
            load_obj_by_material(std::string("models/Trees/OBJ/") + files[k] + ".obj", pick);
        kinds.push_back(place_model(*mesh, heights[k], rt::point3f(0, 0, 0), 0.0f, 0.0f));
    }
    return kinds;
}

// Whether a tree at (x, z) would stand on the plaza or the path
bool on_paving(float x, float z, const layout& site)
{
    if (std::fabs(x) < site.plaza_x + 6 && std::fabs(z) < site.plaza_z + 6)
    {
        return true;
    }
    const rt::vec3f offset = rt::point3f(x, 0, z) - site.path_start;
    return dot(offset, site.path_direction) > 0 &&
           std::fabs(dot(offset, site.path_across)) < site.path_width / 2 + 4;
}

/*
 * Trees on a jittered grid `spacing` apart, between square "radii" `inner` and `outer` around the
 * towers. Clearings come from a coarse random field, and kinds grow in patches. Past `inner` > 0,
 * trees are only planted within the camera's view.
 */
void plant_grid(rt::hittable_list& world, const std::vector<shared_ptr<rt::hittable>>& kinds,
                const layout& site, float spacing, float inner, float outer, int seed)
{
    const rt::vec3f view = flat_view(site);
    const int kind_count = static_cast<int>(kinds.size());
    const int cells      = static_cast<int>(outer / spacing);
    for (int a = -cells; a <= cells; ++a)
    {
        for (int b = -cells; b <= cells; ++b)
        {
            const float x = (a + 0.8f * (hash(a, b, seed + 1) - 0.5f)) * spacing;
            const float z = (b + 0.8f * (hash(a, b, seed + 2) - 0.5f)) * spacing;
            const float r = std::max(std::fabs(x), std::fabs(z));
            if (r < inner || r > outer || on_paving(x, z, site))
            {
                continue;
            }
            if (inner > 0)
            {
                const rt::vec3f from_camera =
                    unit_vector(rt::vec3f(x - site.look_from.x(), 0, z - site.look_from.z()));
                if (dot(from_camera, view) < std::cos(0.75f))
                {
                    continue;
                }
            }
            if (hash(a / 6, b / 6, seed + 3) < 0.18f && hash(a, b, seed + 4) < 0.85f) // clearing
            {
                continue;
            }
            const float patch = hash(static_cast<int>(std::floor(a / 5.0f)),
                                     static_cast<int>(std::floor(b / 5.0f)), seed + 7);
            const float pick  = hash(a, b, seed + 8) < 0.7f ? patch : hash(a, b, seed + 5);
            const int kind    = std::min(kind_count - 1, static_cast<int>(kind_count * pick));
            world.add(make_shared<rt::translate>(
                make_shared<rt::rotate_y>(kinds[kind], 360.0f * hash(a, b, seed + 6)),
                rt::vec3f(x, 0, z)));
        }
    }
}

// Dense woods around the towers, then sparser ones (crowns merge at a distance) to the horizon
void plant_trees(rt::hittable_list& world, const layout& site)
{
    const auto kinds = load_tree_kinds();
    plant_grid(world, kinds, site, 11, 0, 500, 0);
    plant_grid(world, kinds, site, 24, 500, 5500, 10);
}

/*
 * A pink sunset sky (Belfast Sunset, Poly Haven, CC0), dimmed so it doesn't wash out and turned so
 * its afterglow (u = 0.605) is behind the towers. Its sun has set, so a separate rose-gold sun
 * lights the scene.
 */
shared_ptr<rt::sky_image> add_sky_and_sun(rt::hittable_list& world, rt::hittable_list& lights,
                                          const layout& site)
{
    const rt::vec3f view = flat_view(site);
    const rt::vec3f left(view.z(), 0, -view.x());
    auto heading = [&](float radians_left) // a horizontal direction, turned left from the view
    {
        return std::cos(radians_left) * view + std::sin(radians_left) * left;
    };

    const rt::vec3f glow = heading(-0.2f);
    const float sky_turn = 2.0f * rt::PI * 0.605f - std::atan2(glow.z(), glow.x());
    auto sky = make_shared<rt::sky_image>("texture/belfast_sunset_puresky_4k.hdr", 0.25f,
                                          sky_turn * 180.0f / rt::PI);

    // A sphere the sun's size (0.53 degrees) far away, as bright as a clear day's sun
    const float elevation = 32.0f * rt::PI / 180.0f, half_angle = 0.00465f, distance = 10000;
    const rt::point3f center =
        rt::point3f(0, 0, 0) +
        distance * (std::cos(elevation) * heading(2.5f) + std::sin(elevation) * rt::vec3f(0, 1, 0));
    const float radius      = distance * std::tan(half_angle);
    const float solid_angle = 2.0f * rt::PI * (1.0f - std::cos(half_angle));
    auto sunlight = make_shared<rt::diffuse_light>((1.0f / solid_angle) * rt::color(4.7, 4.1, 3.8));
    world.add(make_shared<rt::sphere>(center, radius, sunlight));
    lights.add(make_shared<rt::sphere>(center, radius, shared_ptr<rt::material>()));
    return sky;
}

} // namespace

void building(const std::filesystem::path& output_path)
{
    const layout site;
    rt::hittable_list world, lights;
    add_towers(world, site);
    add_ground(world, site);
    plant_trees(world, site);
    auto sky = add_sky_and_sun(world, lights, site);
    world    = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 3.0f / 2.0f;
    camera.image_width       = 1200;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 16;
    camera.sky               = sky;
    camera.field_of_view     = 45;
    camera.look_from         = site.look_from;
    camera.look_at           = site.look_at;
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.output_path       = output_path;
    camera.defocus_angle     = 0;
    camera.render(world, lights);
}

} // namespace scenes
