#include "scripting/scene_script.h"

#include <cstdio>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "scripting/binding.h"

namespace scripting
{

namespace bindings
{

namespace
{

[[noreturn]] void fail(const std::string& message)
{
    throw std::runtime_error(message);
}

// An error from one of the renderer's functions, reported at the line of the script that called it
int at_script_line(lua_State* L, sol::optional<const std::exception&>, sol::string_view message)
{
    luaL_where(L, 1);
    lua_pushlstring(L, message.data(), message.size());
    lua_concat(L, 2);
    return 1;
}

void set_camera(settings& s, scene_state& scene)
{
    rt::Camera& camera = scene.camera;
    // A number, or "width:height"
    if (const sol::object aspect = s.get("aspect_ratio"); aspect.get_type() == sol::type::string)
    {
        float width = 0, height = 0;
        if (std::sscanf(aspect.as<std::string>().c_str(), "%f:%f", &width, &height) != 2 ||
            width <= 0 || height <= 0)
        {
            fail("camera: \"aspect_ratio\" should be a number or \"width:height\", like \"16:9\"");
        }
        camera.aspect_ratio = width / height;
    }
    else if (aspect.valid())
    {
        camera.aspect_ratio = to_number(aspect, s.what("aspect_ratio"));
    }
    camera.image_width       = s.integer("image_width", camera.image_width);
    camera.samples_per_pixel = s.integer("samples_per_pixel", camera.samples_per_pixel);
    camera.max_depth         = s.integer("max_depth", camera.max_depth);
    camera.field_of_view     = s.number("field_of_view", camera.field_of_view);
    camera.look_from         = s.vector("look_from", camera.look_from);
    camera.look_at           = s.vector("look_at", camera.look_at);
    camera.view_up           = s.vector("view_up", camera.view_up);
    camera.defocus_angle     = s.number("defocus_angle", camera.defocus_angle);
    camera.focus_distance    = s.number("focus_distance", camera.focus_distance);

    // What rays that hit nothing see: a color, or a sky image
    if (s.has("background"))
    {
        camera.background = s.color("background");
    }
    if (const sol::object sky = s.get("sky"); sky.valid())
    {
        if (!sky.is<sky_handle>())
        {
            fail("camera: \"sky\" should be a sky_image{...}, not " + describe(sky));
        }
        camera.sky = sky.as<sky_handle&>().sky;
    }
}

} // namespace

void register_scene(sol::state& lua, scene_state& scene)
{
    // add(shape, ...): put shapes in the scene; returns them
    lua.set_function("add",
                     [&scene](sol::variadic_args shapes)
                     {
                         if (shapes.size() == 0)
                         {
                             fail("add takes the shapes to put in the scene");
                         }
                         int number = 1;
                         for (const sol::object shape : shapes)
                         {
                             scene.world.add(
                                 to_shape(shape, "add: argument " + std::to_string(number++)).shape);
                         }
                         return shapes;
                     });

    // light(shape, weight): the shadow rays aim at this sphere or quad, `weight` (default 1)
    // times as often as at a light of weight 1. Returns the shape.
    lua.set_function("light",
                     [&scene](const sol::object& value, sol::optional<int> weight)
                     {
                         shape_handle& shape = to_shape(value, "light: the first argument");
                         if (shape.type != "sphere" && shape.type != "quad")
                         {
                             fail("light: the shadow rays can aim at a sphere or a quad, not " +
                                  describe(value));
                         }
                         for (int i = 0; i < weight.value_or(1); ++i)
                         {
                             scene.lights.add(shape.shape);
                         }
                         return value;
                     });

    define(lua, "camera",
           [&scene](settings& s)
           {
               if (scene.has_camera)
               {
                   fail("camera is set twice");
               }
               set_camera(s, scene);
               scene.has_camera = true;
               return sol::lua_nil;
           });

    // sky_image{file, strength, rotation_degrees, max_radiance}: an HDR image around the scene
    define(lua, "sky_image",
           [](settings& s)
           {
               return sky_handle{make_shared<rt::sky_image>(
                   s.string("file"), s.number("strength", 1.0f), s.number("rotation_degrees", 0.0f),
                   s.number("max_radiance", std::numeric_limits<float>::infinity()))};
           });

    lua.set_function("output", [&scene](const std::string& name) { scene.output = name; });

    // The renderer's generator, which starts from the same seed every run, so random layouts come
    // out the same; random_vector draws x first
    lua.set_function("random_float", sol::overload([] { return rt::random_float(); },
                                                   [](float low, float high)
                                                   { return rt::random_float(low, high); }));
    lua.set_function("random_vector",
                     sol::overload(
                         [](sol::this_state L) { return make_vector(L, rt::vec3f::random()); },
                         [](float low, float high, sol::this_state L)
                         { return make_vector(L, rt::vec3f::random(low, high)); }));

    // hash_string(text): a fixed whole number for a string, to spread names over choices
    lua.set_function("hash_string",
                     [](const std::string& text)
                     {
                         return static_cast<lua_Integer>(std::hash<std::string>()(text) &
                                                         0x7fffffffffffffffull);
                     });
}

} // namespace bindings

scene_description load_scene(const std::filesystem::path& file)
{
    if (!std::filesystem::is_regular_file(file))
    {
        throw std::runtime_error("no scene script " + file.string());
    }
    bindings::scene_state scene;
    {
        sol::state lua;
        lua.open_libraries();
        sol::set_default_exception_handler(lua.lua_state(), bindings::at_script_line);
        bindings::register_materials(lua);
        bindings::register_shapes(lua);
        bindings::register_scene(lua, scene);

        // require() finds helpers in lib/ next to the script
        const std::filesystem::path folder = file.parent_path().empty() ? "." : file.parent_path();
        lua["package"]["path"]             = (folder / "lib" / "?.lua").string();

        // The prelude gets the vector metatable and the layout file for placed()
        // (11_dragon.lua has 11_dragon.layout.lua), and returns a function that lists the layout's
        // names nothing asked for
        std::filesystem::path layout = file;
        layout.replace_extension(".layout.lua");
        sol::table vector_metatable = lua.create_table();
        lua.registry()["rt.vec"]    = vector_metatable;
        sol::load_result prelude = lua.load(bindings::PRELUDE, "=prelude");
        if (!prelude.valid())
        {
            throw std::logic_error(sol::error(prelude).what());
        }
        sol::protected_function_result unused_layout =
            prelude.get<sol::protected_function>()(vector_metatable, layout.string());
        if (!unused_layout.valid())
        {
            throw std::runtime_error(sol::error(unused_layout).what());
        }
        sol::protected_function list_unused = unused_layout;

        sol::protected_function_result result =
            lua.safe_script_file(file.string(), sol::script_pass_on_error);
        if (!result.valid())
        {
            throw std::runtime_error(sol::error(result).what());
        }
        sol::table unused = list_unused();
        for (const auto& [index, name] : unused)
        {
            std::cerr << layout.string() << ": warning: nothing in the scene is placed(\""
                      << name.as<std::string>() << "\")\n";
        }
    } // Lua's copies of the scene's objects are released here

    if (!scene.has_camera)
    {
        throw std::runtime_error(file.string() + ": the script never calls camera{...}");
    }
    if (scene.world.objects.empty())
    {
        throw std::runtime_error(file.string() + ": the script adds nothing to the scene");
    }
    scene_description result;
    result.world  = rt::hittable_list(rt::make_shared<rt::bvh_node>(scene.world));
    result.lights = std::move(scene.lights);
    result.camera = scene.camera;
    result.output = scene.output.empty() ? file.stem().string() + ".png" : scene.output;
    return result;
}

void render_scene(const std::filesystem::path& file, const std::filesystem::path& output_directory)
{
    scene_description scene  = load_scene(file);
    scene.camera.output_path = output_directory / scene.output;
    if (scene.lights.objects.empty())
    {
        scene.camera.render(scene.world);
    }
    else
    {
        scene.camera.render(scene.world, scene.lights);
    }
}

} // namespace scripting
