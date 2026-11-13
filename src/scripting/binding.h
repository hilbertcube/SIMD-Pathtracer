#pragma once

#include <set>
#include <string>

#include <sol/sol.hpp>

#include "rt/ray_tracer.h"
#include "rt/render/sky.h"

/*
 * The renderer as Lua sees it, through sol2. Scripts make textures, materials, shapes and skies
 * by calling functions with a table of settings, like sphere{center = ..., radius = ...}; each
 * area of the API registers its functions in its own file.
 */
namespace scripting::bindings
{

using rt::make_shared;
using rt::shared_ptr;

// What scripts hold. Each remembers the function that made it, for messages.
struct texture_handle
{
    shared_ptr<rt::texture> texture;
};

struct material_handle
{
    shared_ptr<rt::material> material;
    std::string type;
};

struct shape_handle
{
    shared_ptr<rt::hittable> shape;
    std::string type;
    shared_ptr<rt::hittable_list> mesh; // a model's triangles where they stand, for its queries
};

struct sky_handle
{
    shared_ptr<const rt::sky_image> sky;
};

// The scene a script is building
struct scene_state
{
    rt::hittable_list world;
    rt::hittable_list lights;
    rt::Camera camera;
    bool has_camera = false;
    std::string output;
};

// Values from Lua. Each throws a std::runtime_error naming `what`, e.g. `sphere: "radius"`, for a
// value of the wrong kind; sol2 reports it at the script's line.
std::string describe(const sol::object& value); // "a number", "a material (lambertian)"
bool is_vector(const sol::object& value);
bool is_color(const sol::object& value);
float to_number(const sol::object& value, const std::string& what);
rt::vec3f to_vector(const sol::object& value, const std::string& what);   // {x, y, z}
rt::color to_color(const sol::object& value, const std::string& what);    // {r, g, b}, or a grey
shared_ptr<rt::texture> to_texture(const sol::object& value, const std::string& what); // or a color
shared_ptr<rt::material> to_material(const sol::object& value, const std::string& what); // or nil
shape_handle& to_shape(const sol::object& value, const std::string& what);

// A vector for scripts: a {x, y, z} table with the vector metatable, so it has arithmetic
sol::table make_vector(sol::state_view lua, const rt::vec3f& value);

/*
 * The table of settings a function takes. Each getter reads one key; finish() reports keys nobody
 * read, so a misspelled one is an error, and a missing key suggests a near spelling.
 */
class settings
{
public:
    settings(const sol::object& table, std::string function);

    sol::object get(const char* key); // nil if not set
    sol::object required(const char* key);
    bool has(const char* key) { return get(key) != sol::lua_nil; }
    std::string what(const char* key) const { return function + ": \"" + key + "\""; }

    float number(const char* key) { return to_number(required(key), what(key)); }
    float number(const char* key, float fallback);
    int integer(const char* key, int fallback);
    bool boolean(const char* key, bool fallback);
    std::string string(const char* key);
    rt::vec3f vector(const char* key) { return to_vector(required(key), what(key)); }
    rt::vec3f vector(const char* key, const rt::vec3f& fallback);
    rt::color color(const char* key) { return to_color(required(key), what(key)); }
    shared_ptr<rt::texture> texture(const char* key) { return to_texture(required(key), what(key)); }
    shared_ptr<rt::material> material(const char* key) { return to_material(required(key), what(key)); }
    shared_ptr<rt::material> optional_material(const char* key) { return to_material(get(key), what(key)); }

    void finish();

private:
    sol::table table;
    std::string function;
    std::set<std::string> read;
};

// Register the function `name`, taking a table of settings that `build` turns into what it returns
template <typename Build>
void define(sol::state& lua, const std::string& name, Build build)
{
    lua.set_function(name,
                     [name, build](const sol::object& table)
                     {
                         settings s(table, name);
                         auto result = build(s);
                         s.finish();
                         return result;
                     });
}

// Each area of the API puts its functions in Lua's globals
void register_materials(sol::state& lua); // textures and materials
void register_shapes(sol::state& lua);    // shapes and models
void register_scene(sol::state& lua, scene_state& scene);

// Lua code run before the script: vectors, and placed() (prelude.cpp)
extern const char* const PRELUDE;

} // namespace scripting::bindings
