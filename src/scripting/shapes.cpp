#include <stdexcept>
#include <string>
#include <unordered_map>

#include "scripting/load_model.h"
#include "scripting/load_obj_by_material.h"
#include "scripting/binding.h"

// Shapes and models for scripts
namespace scripting::bindings
{

namespace
{

[[noreturn]] void fail(const std::string& message)
{
    throw std::runtime_error(message);
}

/*
 * A model's triangles, from `file`: with "parts" (a table of usemtl names to materials, or a
 * function of the name), each part gets its own; otherwise "loader" says how to read it
 */
shared_ptr<rt::hittable_list> load_mesh(settings& s, const std::string& file,
                                        shared_ptr<rt::material> fallback)
{
    const sol::object parts = s.get("parts");
    if (!parts.valid())
    {
        const std::string loader = s.has("loader") ? s.string("loader") : "flat";
        if (loader == "flat")
        {
            return rt::load_obj(file, fallback);
        }
        if (loader == "textured")
        {
            return rt::load_obj_with_attributes(file, fallback);
        }
        if (loader == "smooth")
        {
            return rt::load_obj_smooth(file, fallback, s.number("crease_degrees", 45.0f));
        }
        fail("model: unknown \"loader\" \"" + loader + "\" (flat, textured or smooth)");
    }
    if (s.has("loader"))
    {
        fail("model: a model with \"parts\" always keeps its texture coordinates and normals, so "
             "it takes no \"loader\"");
    }

    if (parts.get_type() == sol::type::function)
    {
        const sol::function pick = parts;
        return load_obj_by_material(file,
                                    [&](const std::string& name)
                                    {
                                        const sol::object chosen = pick(name);
                                        auto material = to_material(
                                            chosen, "model: what \"parts\" gave for \"" + name + "\"");
                                        return material ? material : fallback;
                                    });
    }
    if (parts.get_type() != sol::type::table)
    {
        fail("model: \"parts\" should be a table of part names to materials, or a function, not " +
             describe(parts));
    }
    std::unordered_map<std::string, shared_ptr<rt::material>> by_name;
    for (const auto& [name, material] : parts.as<sol::table>())
    {
        const std::string part = name.as<std::string>();
        by_name[part]          = to_material(material, "model: the part \"" + part + "\"");
    }
    std::set<std::string> found;
    auto mesh = load_obj_by_material(file,
                                     [&](const std::string& name)
                                     {
                                         found.insert(name);
                                         auto part = by_name.find(name);
                                         return part != by_name.end() ? part->second : fallback;
                                     });
    for (const auto& part : by_name)
    {
        if (!found.count(part.first))
        {
            fail("model: " + file + " has no part \"" + part.first + "\"");
        }
    }
    return mesh;
}

/*
 * face_materials = {texture, pick}: pick(color) gives the material of the triangles whose texture
 * color, at their middle, is `color`, or nil to leave them as they are
 */
void apply_face_materials(const sol::object& value, rt::hittable_list& mesh)
{
    settings faces(value, "model: face_materials");
    const shared_ptr<rt::texture> colors = faces.texture("texture");
    const sol::object pick_value         = faces.required("pick");
    if (pick_value.get_type() != sol::type::function)
    {
        fail("model: face_materials \"pick\" should be a function(color)");
    }
    faces.finish();
    const sol::function pick = pick_value;
    for (const auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<rt::attributed_triangle>(object);
        if (!face)
        {
            continue;
        }
        const float u = (face->texture_u[0] + face->texture_u[1] + face->texture_u[2]) / 3.0f;
        const float v = (face->texture_v[0] + face->texture_v[1] + face->texture_v[2]) / 3.0f;
        const rt::color texel   = colors->value(u, v, rt::point3f(0, 0, 0));
        const sol::object given = pick(make_vector(pick.lua_state(), rt::vec3f(texel.r(), texel.g(), texel.b())));
        if (auto material = to_material(given, "model: what face_materials \"pick\" gave"))
        {
            face->surface_material = material;
        }
    }
}

/*
 * model{file, material, ...}: an OBJ file in its own BVH. Besides "parts", "loader" and
 * "face_materials" (above), it is either stood `height` tall on `base`, turned about x by tilt_x
 * and then about y by turn_y, or scaled by `scale` (a number or vector) and moved by `offset`.
 */
shape_handle make_model(settings& s)
{
    const std::string file = s.string("file");
    auto mesh              = load_mesh(s, file, s.optional_material("material"));
    if (mesh->objects.empty())
    {
        fail("model: " + file + " has no triangles");
    }
    if (const sol::object faces = s.get("face_materials"); faces.valid())
    {
        apply_face_materials(faces, *mesh);
    }

    if (s.has("height"))
    {
        if (s.has("scale") || s.has("offset"))
        {
            fail("model: give either \"height\" (with \"base\", \"turn_y\", \"tilt_x\") or "
                 "\"scale\" and \"offset\"");
        }
        auto shape = place_model(*mesh, s.number("height"), s.vector("base", rt::vec3f(0, 0, 0)),
                                 s.number("turn_y", 0.0f), s.number("tilt_x", 0.0f));
        return {shape, "model", mesh};
    }
    if (s.has("turn_y") || s.has("tilt_x") || s.has("base"))
    {
        fail("model: \"base\", \"turn_y\" and \"tilt_x\" go with \"height\"");
    }
    const rt::vec3f offset  = s.vector("offset", rt::vec3f(0, 0, 0));
    const sol::object scale = s.get("scale");
    if (scale.get_type() == sol::type::number)
    {
        rt::transform_mesh(*mesh, scale.as<float>(), offset);
    }
    else if (scale.valid())
    {
        rt::transform_mesh(*mesh, to_vector(scale, s.what("scale")), offset);
    }
    else if (s.has("offset"))
    {
        rt::transform_mesh(*mesh, 1.0f, offset);
    }
    return {make_shared<rt::bvh_node>(mesh->objects, 0, mesh->objects.size()), "model", mesh};
}

// Every vertex of a model, for its methods
template <typename Visit>
void each_vertex(const shape_handle& model, const char* method, Visit visit)
{
    if (!model.mesh)
    {
        fail(std::string(method) + " is a method of models, not of a " + model.type);
    }
    for (const auto& object : model.mesh->objects)
    {
        if (auto face = std::dynamic_pointer_cast<rt::mesh_triangle>(object))
        {
            visit(face->vertex0);
            visit(face->vertex1);
            visit(face->vertex2);
        }
    }
}

// model:frontmost(direction): the vertex that lies farthest along `direction`
sol::table frontmost(const shape_handle& model, const sol::object& direction, sol::this_state lua)
{
    const rt::vec3f along = to_vector(direction, "frontmost: the direction");
    rt::point3f best;
    float best_distance = -rt::INF;
    each_vertex(model, "frontmost",
                [&](const rt::point3f& vertex)
                {
                    if (dot(along, vertex) > best_distance)
                    {
                        best_distance = dot(along, vertex);
                        best          = vertex;
                    }
                });
    return make_vector(lua, best);
}

// model:bounds(): the corners of the box around it, min then max
std::tuple<sol::table, sol::table> bounds(const shape_handle& model, sol::this_state lua)
{
    rt::point3f low(rt::INF, rt::INF, rt::INF), high(-rt::INF, -rt::INF, -rt::INF);
    each_vertex(model, "bounds",
                [&](const rt::point3f& vertex)
                {
                    for (int axis = 0; axis < 3; ++axis)
                    {
                        low[axis]  = std::fmin(low[axis], vertex[axis]);
                        high[axis] = std::fmax(high[axis], vertex[axis]);
                    }
                });
    return {make_vector(lua, low), make_vector(lua, high)};
}

} // namespace

void register_shapes(sol::state& lua)
{
    // The methods shapes have (only models answer them); no global is left behind
    lua.new_usertype<shape_handle>("shape", sol::no_constructor, "frontmost", &frontmost, "bounds",
                                   &bounds);
    lua["shape"] = sol::lua_nil;

    define(lua, "sphere",
           [](settings& s)
           {
               const rt::point3f center = s.vector("center");
               const float radius       = s.number("radius");
               auto material            = s.optional_material("material");
               if (s.has("center2")) // moving from center to center2 while the shutter is open
               {
                   return shape_handle{
                       make_shared<rt::sphere>(center, s.vector("center2"), radius, material),
                       "sphere"};
               }
               return shape_handle{make_shared<rt::sphere>(center, radius, material), "sphere"};
           });
    define(lua, "quad",
           [](settings& s) {
               return shape_handle{make_shared<rt::quad>(s.vector("corner"), s.vector("u"),
                                                         s.vector("v"),
                                                         s.optional_material("material")),
                                   "quad"};
           });
    define(lua, "box",
           [](settings& s) {
               return shape_handle{
                   rt::box(s.vector("min"), s.vector("max"), s.optional_material("material")),
                   "box"};
           });
    define(lua, "pyramid",
           [](settings& s) {
               return shape_handle{rt::pyramid(s.vector("base_min"), s.vector("base_max"),
                                               s.vector("apex"), s.optional_material("material")),
                                   "pyramid"};
           });
    define(lua, "model", make_model);

    // constant_medium{boundary, density, albedo or phase}: fog, smoke or a scattering medium
    define(lua, "constant_medium",
           [](settings& s)
           {
               auto boundary       = to_shape(s.required("boundary"), s.what("boundary")).shape;
               const float density = s.number("density");
               if (s.has("albedo") == s.has("phase"))
               {
                   fail("constant_medium: give either \"albedo\" (a color or texture) or \"phase\" "
                        "(a material)");
               }
               shared_ptr<rt::hittable> medium =
                   s.has("phase")
                       ? make_shared<rt::constant_medium>(boundary, density, s.material("phase"))
                       : make_shared<rt::constant_medium>(boundary, density, s.texture("albedo"));
               return shape_handle{medium, "constant_medium"};
           });

    // bvh_node({shape, ...}): shapes in their own BVH, which then move and turn together
    lua.set_function("bvh_node",
                     [](const sol::object& list)
                     {
                         if (list.get_type() != sol::type::table || list.as<sol::table>().size() == 0)
                         {
                             fail("bvh_node takes a list of shapes, bvh_node({a, b, ...}), not " +
                                  describe(list));
                         }
                         rt::hittable_list shapes;
                         for (const auto& [index, shape] : list.as<sol::table>())
                         {
                             shapes.add(to_shape(shape, "bvh_node: item " +
                                                            std::to_string(index.as<int>()))
                                            .shape);
                         }
                         return shape_handle{make_shared<rt::bvh_node>(shapes), "bvh_node"};
                     });

    // rotate_y(shape, degrees), about the y axis through the origin; translate(shape, offset)
    lua.set_function("rotate_y",
                     [](const sol::object& shape, const sol::object& degrees)
                     {
                         return shape_handle{
                             make_shared<rt::rotate_y>(to_shape(shape, "rotate_y: the shape").shape,
                                                       to_number(degrees, "rotate_y: the angle")),
                             "rotate_y"};
                     });
    lua.set_function("translate",
                     [](const sol::object& shape, const sol::object& offset)
                     {
                         return shape_handle{make_shared<rt::translate>(
                                                 to_shape(shape, "translate: the shape").shape,
                                                 to_vector(offset, "translate: the offset")),
                                             "translate"};
                     });
}

} // namespace scripting::bindings
