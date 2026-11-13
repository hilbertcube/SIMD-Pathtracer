#include "scripting/binding.h"

// Textures and materials for scripts: one function per class, named after it
namespace scripting::bindings
{

namespace
{

// An image multiplied by a color: with a black-and-white image, the white parts take the color
class tinted_image : public rt::texture
{
public:
    tinted_image(shared_ptr<rt::texture> image, const rt::color& tint) : image(image), tint(tint) {}

    rt::color value(float u, float v, const rt::point3f& point) const override
    {
        return tint * image->value(u, v, point);
    }

private:
    shared_ptr<rt::texture> image;
    rt::color tint;
};

// A material whose `key` can be a color or a texture, made with the matching constructor
template <typename Material, typename... Extra>
shared_ptr<rt::material> by_albedo(settings& s, const char* key, Extra... extra)
{
    if (is_color(s.get(key)))
    {
        return make_shared<Material>(s.color(key), extra...);
    }
    return make_shared<Material>(s.texture(key), extra...);
}

template <typename Build>
void define_texture(sol::state& lua, const char* name, Build build)
{
    define(lua, name, [build](settings& s) { return texture_handle{build(s)}; });
}

template <typename Build>
void define_material(sol::state& lua, const char* name, Build build)
{
    define(lua, name, [name, build](settings& s) { return material_handle{build(s), name}; });
}

} // namespace

void register_materials(sol::state& lua)
{
    define_texture(lua, "checker_texture",
                   [](settings& s) -> shared_ptr<rt::texture> {
                       return make_shared<rt::checker_texture>(s.number("scale"), s.texture("even"),
                                                               s.texture("odd"));
                   });
    define_texture(lua, "image_texture",
                   [](settings& s)
                   {
                       shared_ptr<rt::texture> image =
                           make_shared<rt::image_texture>(s.string("file").c_str());
                       if (s.has("tint")) // multiplies the image by a color
                       {
                           image = make_shared<tinted_image>(image, s.color("tint"));
                       }
                       return image;
                   });
    define_texture(lua, "tiled_texture",
                   [](settings& s) -> shared_ptr<rt::texture> {
                       return make_shared<rt::tiled_texture>(s.texture("texture"),
                                                             s.number("repeat_u"),
                                                             s.number("repeat_v"));
                   });
    define_texture(lua, "noise_texture",
                   [](settings& s) -> shared_ptr<rt::texture> {
                       return make_shared<rt::noise_texture>(s.number("scale"),
                                                             s.number("frequency", 1.0f));
                   });

    define_material(lua, "lambertian",
                    [](settings& s) { return by_albedo<rt::lambertian>(s, "albedo"); });
    define_material(lua, "oren_nayar", [](settings& s)
                    { return by_albedo<rt::oren_nayar>(s, "albedo", s.number("sigma")); });
    define_material(lua, "coated", [](settings& s)
                    { return by_albedo<rt::coated>(s, "albedo", s.number("coat_index", 1.5f)); });
    define_material(lua, "diffuse_light",
                    [](settings& s) { return by_albedo<rt::diffuse_light>(s, "emission"); });
    define_material(lua, "isotropic",
                    [](settings& s) { return by_albedo<rt::isotropic>(s, "albedo"); });
    define_material(lua, "velvet",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::velvet>(s.color("albedo"), s.color("sheen"),
                                                       s.number("power", 3.0f),
                                                       s.number("strength", 1.0f));
                    });
    define_material(lua, "metal",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::metal>(s.color("albedo"), s.number("fuzz", 0.0f));
                    });
    define_material(lua, "conductor",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::conductor>(s.color("reflectance"),
                                                          s.number("roughness", 0.0f));
                    });
    define_material(lua, "brushed_metal",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::brushed_metal>(
                            s.color("reflectance"), s.number("roughness_along"),
                            s.number("roughness_across"), s.vector("axis", rt::vec3f(0, 1, 0)));
                    });
    define_material(lua, "metallic_roughness",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::metallic_roughness>(
                            s.texture("base_color"), s.texture("metallic"), s.texture("roughness"));
                    });
    define_material(lua, "dielectrics",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::dielectrics>(s.number("refraction_index"));
                    });
    define_material(lua, "tinted_dielectric",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::tinted_dielectric>(
                            s.number("refraction_index"), s.color("tint"), s.number("distance"));
                    });
    define_material(lua, "rough_dielectric",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::rough_dielectric>(s.number("refraction_index"),
                                                                 s.number("roughness"));
                    });
    define_material(lua, "dispersive_dielectric",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::dispersive_dielectric>(
                            s.number("red_index"), s.number("green_index"), s.number("blue_index"));
                    });
    define_material(lua, "iridescent",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::iridescent>(
                            s.color("albedo"), s.number("thickness_nm", 480.0f),
                            s.number("film_index", 1.4f), s.number("coat_reflectance", 0.3f));
                    });
    define_material(lua, "bumped",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::bumped>(s.material("base"), s.number("feature_scale"),
                                                       s.number("height"));
                    });
    define_material(lua, "cutout",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::cutout>(s.material("base"), s.string("mask"),
                                                       s.number("threshold", 0.5f));
                    });
    define_material(lua, "mist",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::mist>(s.color("albedo"), s.number("g"));
                    });
    define_material(lua, "glowing_medium",
                    [](settings& s) -> shared_ptr<rt::material> {
                        return make_shared<rt::glowing_medium>(s.color("albedo"),
                                                               s.color("emission"));
                    });
}

} // namespace scripting::bindings
