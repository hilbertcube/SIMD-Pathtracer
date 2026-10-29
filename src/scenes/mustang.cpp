#include "scenes/scenes.h"

#include <string>

#include "rt/geometry/mesh.h"
#include "rt/materials/materials.h"
#include "rt/ray_tracer.h"
#include "scenes/load_obj_by_material.h"

namespace scenes
{

using std::make_shared;
using std::shared_ptr;

namespace
{

// An image multiplied by a color: with a black-and-white stripe image, the white parts take the
// color and the black parts stay black
class tinted_image : public rt::texture
{
public:
    tinted_image(const char* filename, const rt::color& tint)
        : image(make_shared<rt::image_texture>(filename)), tint(tint)
    {
    }

    rt::color value(float u, float v, const rt::point3f& point) const override
    {
        return tint * image->value(u, v, point);
    }

private:
    shared_ptr<rt::image_texture> image;
    rt::color tint;
};

// The Mustang's materials, chosen by the part names in the model. Its own material file only
// lists the names, so every part gets its look here.
shared_ptr<rt::material> mustang_material(const std::string& name)
{
    const rt::color paint(0.85, 0.20, 0.02); // Competition Orange
    const rt::color chrome(0.95, 0.95, 0.97);

    if (name == "M_Paintjob1") // the body, with black Boss 302 stripes
    {
        return make_shared<rt::coated>(
            make_shared<tinted_image>("models/Mustang/MustangBodyStripe1.png", paint), 1.6f);
    }
    if (name == "M_PaintJob2")
    {
        return make_shared<rt::coated>(
            make_shared<tinted_image>("models/Mustang/MustangBodyStripe2.png", paint), 1.6f);
    }
    if (name == "M_ChromeTrim" || name == "M_ChromeTrim2" || name == "M_Mirror1")
    {
        return make_shared<rt::conductor>(chrome, 0.0f);
    }
    if (name == "M_GrillTrim2" || name == "M_HeadLight1")
    {
        return make_shared<rt::conductor>(chrome, 0.05f);
    }
    if (name == "M_Rim1" || name == "M_Rim2")
    {
        return make_shared<rt::conductor>(rt::color(0.80, 0.81, 0.84), 0.18f);
    }
    if (name == "M_Tire1")
    {
        return make_shared<rt::oren_nayar>(rt::color(0.025, 0.025, 0.03), 0.6f);
    }
    if (name == "M_BrakePad1" || name == "M_BrakePad2")
    {
        return make_shared<rt::conductor>(rt::color(0.45, 0.45, 0.48), 0.45f);
    }
    if (name == "M_Exhaust")
    {
        return make_shared<rt::conductor>(rt::color(0.55, 0.55, 0.58), 0.3f);
    }
    if (name == "M_Glass1" || name == "M_WindShield" || name == "M_HeadLight2")
    {
        return make_shared<rt::tinted_dielectric>(1.5f, rt::color(0.55, 0.62, 0.60), 40.0f);
    }
    if (name == "M_Tailight1" || name == "M_Tailight2")
    {
        return make_shared<rt::coated>(rt::color(0.65, 0.02, 0.02));
    }
    if (name == "M_TurnSignalLight1")
    {
        return make_shared<rt::coated>(rt::color(0.9, 0.45, 0.05));
    }
    // Black trim, the grille, the underside: dark plastic and rubber
    return make_shared<rt::coated>(rt::color(0.03, 0.03, 0.035));
}

} // namespace

// A 1969-style Mustang in a dark studio, on a glossy floor, lit by softboxes
void mustang(const std::filesystem::path& output_path)
{
    rt::hittable_list world;
    rt::hittable_list lights; // what the shadow rays aim at: geometry only, materials are ignored

    // The model is in meters, facing +x, with its wheels at y = -0.36 and its middle at about
    // (-0.55, 0, 0.12); the scene is in centimeters with the wheels on the floor
    auto car = load_obj_by_material("models/Mustang/Mustang.obj", mustang_material);
    rt::transform_mesh(*car, 100.0f, rt::vec3f(55, 36, -12));
    world.add(make_shared<rt::bvh_node>(car->objects, 0, car->objects.size()));

    // A glossy dark floor, so the car is reflected in it
    world.add(make_shared<rt::quad>(rt::point3f(-4000, 0, -4000), rt::vec3f(8000, 0, 0),
                                    rt::vec3f(0, 0, 8000),
                                    make_shared<rt::coated>(rt::color(0.05, 0.05, 0.06))));

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

    // A big softbox overhead, and two out of the picture (above and behind the camera) that
    // light the front and the side and make long highlights down the body
    add_light(rt::point3f(-600, 500, -500), rt::vec3f(1200, 0, 0), rt::vec3f(0, 0, 1000),
              rt::color(3.0f, 3.0f, 3.2f), 3);
    add_light(rt::point3f(-100, 450, -1000), rt::vec3f(800, 0, 0), rt::vec3f(0, 0, 600),
              rt::color(3.0f, 3.0f, 3.4f), 2);
    add_light(rt::point3f(900, 20, -1000), rt::vec3f(0, 0, 500), rt::vec3f(0, 300, 0),
              rt::color(3.0f, 3.0f, 3.4f), 2);

    world = rt::hittable_list(make_shared<rt::bvh_node>(world));

    rt::Camera camera;
    camera.aspect_ratio      = 16.0f / 9.0f;
    camera.image_width       = 1280;
    camera.samples_per_pixel = 1000;
    camera.max_depth         = 50;
    camera.background        = rt::color(0.01, 0.01, 0.015);
    camera.field_of_view     = 34;
    camera.look_from         = rt::point3f(560, 140, -620);
    camera.look_at           = rt::point3f(0, 60, 0);
    camera.view_up           = rt::vec3f(0, 1, 0);
    camera.defocus_angle     = 0;
    camera.output_path       = output_path;
    camera.render(world, lights);
}

} // namespace scenes
