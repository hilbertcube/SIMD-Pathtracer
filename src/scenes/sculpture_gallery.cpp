#include "scenes/scenes.h"

#include "rt/ray_tracer.h"
#include "scenes/load_model.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

// Six statues on marble pedestals under spotlights, in two staggered rows. In front: stone Lucy, a
// gold Buddha and a bronze bust. Behind: a terracotta Demosthenes, a silver Nefertiti and a jade
// lion.
void sculpture_gallery(const std::filesystem::path& output_path)
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

    const float pedestal_height = 140;
    rt::hittable_list lights;

    // A pedestal centered at (x, z) with a spotlight panel above it, facing down (edge_u x edge_v
    // points to -y); returns the middle of the pedestal's top face
    auto add_pedestal = [&](float x, float z, float width, float depth)
    {
        world.add(rt::box(rt::point3f(x - width / 2, 0, z - depth / 2),
                          rt::point3f(x + width / 2, pedestal_height, z + depth / 2), marble));

        rt::point3f corner(x - 45, 499, z - 45);
        world.add(
            make_shared<rt::quad>(corner, rt::vec3f(90, 0, 0), rt::vec3f(0, 0, 90), spotlight));
        lights.add(make_shared<rt::quad>(corner, rt::vec3f(90, 0, 0), rt::vec3f(0, 0, 90),
                                         shared_ptr<rt::material>()));
        return rt::point3f(x, pedestal_height, z);
    };

    auto stone      = make_shared<rt::lambertian>(rt::color(0.85, 0.83, 0.78));
    auto gold       = make_shared<rt::metal>(rt::color(0.95, 0.72, 0.30), 0.2f);
    auto bronze     = make_shared<rt::metal>(rt::color(0.55, 0.35, 0.20), 0.35f);
    auto terracotta = make_shared<rt::lambertian>(rt::color(0.76, 0.40, 0.26));
    auto silver     = make_shared<rt::metal>(rt::color(0.90, 0.90, 0.93), 0.03f);
    auto jade       = make_shared<rt::lambertian>(rt::color(0.25, 0.60, 0.42));

    // Front row
    world.add(
        load_model("model/lucy.obj", stone, 230, add_pedestal(160, 280, 120, 120), 0.0f, -90.0f));
    world.add(load_model("model/happy.obj", gold, 200, add_pedestal(410, 280, 120, 120)));
    world.add(load_model("model/max-planck.obj", bronze, 190, add_pedestal(630, 280, 120, 120)));

    // Back row, offset so each statue stands between two in front. The lion is long, so its
    // pedestal is too.
    world.add(load_model("model/demosthenes-low-res.obj", terracotta, 190,
                         add_pedestal(265, 500, 120, 120), 160.0f));
    world.add(load_model("model/nefertiti.obj", silver, 200, add_pedestal(525, 500, 120, 140),
                         180.0f, -90.0f));
    world.add(
        load_model("model/lionstatue.obj", jade, 100, add_pedestal(805, 495, 110, 170), 210.0f));

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0, 0, 0);
    camera.field_of_view     = 36;
    camera.look_from         = rt::point3f(450, 280, -700);
    camera.look_at           = rt::point3f(450, 210, 420);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
