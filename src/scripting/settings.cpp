#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "scripting/binding.h"

// Reading values and settings from Lua, with messages that name what was wrong
namespace scripting::bindings
{

namespace
{

[[noreturn]] void fail(const std::string& message)
{
    throw std::runtime_error(message);
}

// How many letters must be added, removed or changed to turn `a` into `b`
size_t edit_distance(const std::string& a, const std::string& b)
{
    std::vector<size_t> row(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j)
    {
        row[j] = j;
    }
    for (size_t i = 1; i <= a.size(); ++i)
    {
        size_t diagonal = row[0];
        row[0]          = i;
        for (size_t j = 1; j <= b.size(); ++j)
        {
            const size_t above = row[j];
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] != b[j - 1])});
            diagonal = above;
        }
    }
    return row[b.size()];
}

} // namespace

std::string describe(const sol::object& value)
{
    if (value.is<material_handle>())
    {
        return "a material (" + value.as<material_handle&>().type + ")";
    }
    if (value.is<shape_handle>())
    {
        return "a shape (" + value.as<shape_handle&>().type + ")";
    }
    if (value.is<texture_handle>())
    {
        return "a texture";
    }
    if (value.is<sky_handle>())
    {
        return "a sky";
    }
    if (!value.valid())
    {
        return "nil";
    }
    return std::string("a ") + sol::type_name(value.lua_state(), value.get_type());
}

bool is_vector(const sol::object& value)
{
    if (value.get_type() != sol::type::table)
    {
        return false;
    }
    const sol::table table = value;
    if (table.size() != 3)
    {
        return false;
    }
    for (int i = 1; i <= 3; ++i)
    {
        if (table.raw_get<sol::object>(i).get_type() != sol::type::number)
        {
            return false;
        }
    }
    return true;
}

bool is_color(const sol::object& value)
{
    return value.get_type() == sol::type::number || is_vector(value);
}

float to_number(const sol::object& value, const std::string& what)
{
    if (value.get_type() != sol::type::number)
    {
        fail(what + " should be a number, not " + describe(value));
    }
    return value.as<float>();
}

rt::vec3f to_vector(const sol::object& value, const std::string& what)
{
    if (!is_vector(value))
    {
        fail(what + " should be a vector, {x, y, z} or vec(x, y, z), not " + describe(value));
    }
    const sol::table table = value;
    return rt::vec3f(table.raw_get<float>(1), table.raw_get<float>(2), table.raw_get<float>(3));
}

rt::color to_color(const sol::object& value, const std::string& what)
{
    if (value.get_type() == sol::type::number)
    {
        const float grey = value.as<float>();
        return rt::color(grey, grey, grey);
    }
    if (!is_vector(value))
    {
        fail(what + " should be a color, {r, g, b} or one number for a grey, not " +
             describe(value));
    }
    const rt::vec3f rgb = to_vector(value, what);
    return rt::color(rgb.x(), rgb.y(), rgb.z());
}

shared_ptr<rt::texture> to_texture(const sol::object& value, const std::string& what)
{
    if (is_color(value))
    {
        return make_shared<rt::solid_color>(to_color(value, what));
    }
    if (!value.is<texture_handle>())
    {
        fail(what + " should be a color or a texture, not " + describe(value));
    }
    return value.as<texture_handle&>().texture;
}

shared_ptr<rt::material> to_material(const sol::object& value, const std::string& what)
{
    if (!value.valid())
    {
        return nullptr;
    }
    if (!value.is<material_handle>())
    {
        fail(what + " should be a material, not " + describe(value));
    }
    return value.as<material_handle&>().material;
}

shape_handle& to_shape(const sol::object& value, const std::string& what)
{
    if (!value.is<shape_handle>())
    {
        fail(what + " should be a shape, not " + describe(value));
    }
    return value.as<shape_handle&>();
}

sol::table make_vector(sol::state_view lua, const rt::vec3f& value)
{
    sol::table vector           = lua.create_table_with(1, value.x(), 2, value.y(), 3, value.z());
    vector[sol::metatable_key] = lua.registry()["rt.vec"];
    return vector;
}

settings::settings(const sol::object& value, std::string function) : function(std::move(function))
{
    if (value.get_type() != sol::type::table || is_vector(value))
    {
        fail(this->function + " takes a table of settings, " + this->function + "{...}, not " +
             describe(value));
    }
    table = value;
}

sol::object settings::get(const char* key)
{
    read.insert(key);
    return table.raw_get<sol::object>(key);
}

sol::object settings::required(const char* key)
{
    sol::object value = get(key);
    if (value.valid())
    {
        return value;
    }
    std::string message = function + ": missing \"" + key + "\"";
    for (const auto& [name, unused] : table)
    {
        if (name.get_type() == sol::type::string && !read.count(name.as<std::string>()) &&
            edit_distance(name.as<std::string>(), key) <= 2)
        {
            message += " (is \"" + name.as<std::string>() + "\" a misspelling of it?)";
            break;
        }
    }
    fail(message);
}

float settings::number(const char* key, float fallback)
{
    const sol::object value = get(key);
    return value.valid() ? to_number(value, what(key)) : fallback;
}

int settings::integer(const char* key, int fallback)
{
    const float value = number(key, static_cast<float>(fallback));
    if (value != std::floor(value))
    {
        fail(what(key) + " should be a whole number");
    }
    return static_cast<int>(value);
}

bool settings::boolean(const char* key, bool fallback)
{
    const sol::object value = get(key);
    if (!value.valid())
    {
        return fallback;
    }
    if (value.get_type() != sol::type::boolean)
    {
        fail(what(key) + " should be true or false, not " + describe(value));
    }
    return value.as<bool>();
}

std::string settings::string(const char* key)
{
    const sol::object value = required(key);
    if (value.get_type() != sol::type::string)
    {
        fail(what(key) + " should be a string, not " + describe(value));
    }
    return value.as<std::string>();
}

rt::vec3f settings::vector(const char* key, const rt::vec3f& fallback)
{
    const sol::object value = get(key);
    return value.valid() ? to_vector(value, what(key)) : fallback;
}

void settings::finish()
{
    std::string unknown;
    for (const auto& [name, unused] : table)
    {
        if (name.get_type() != sol::type::string)
        {
            fail(function + " takes named settings, like " + function +
                 "{radius = 1}; one of them has no name");
        }
        if (!read.count(name.as<std::string>()))
        {
            unknown += (unknown.empty() ? "\"" : ", \"") + name.as<std::string>() + "\"";
        }
    }
    if (!unknown.empty())
    {
        fail(function + ": unknown setting " + unknown);
    }
}

} // namespace scripting::bindings
