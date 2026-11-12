#pragma once

#include "rt/geometry/hittable_list.h"
#include <algorithm>
#include <array>
#include <map>
#include <vector>

#include "rt/materials/material.h" // for masked_out()
#include "rt/math/vector.h"

#include "tinyobjloader/tiny_obj_loader.h"

namespace rt
{

class mesh_triangle : public hittable
{
public:
    point3f vertex0, vertex1, vertex2;
    shared_ptr<material> surface_material;

    mesh_triangle() {}

    mesh_triangle(const point3f& vertex_a, const point3f& vertex_b, const point3f& vertex_c,
                  shared_ptr<material> triangle_material)
        : vertex0(vertex_a), vertex1(vertex_b), vertex2(vertex_c),
          surface_material(triangle_material)
    {
    }

    // Möller–Trumbore intersection
    // ref: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        const float epsilon = std::numeric_limits<float>::epsilon();
        vec3f edge1         = vertex1 - vertex0;
        vec3f edge2         = vertex2 - vertex0;

        vec3f direction_cross_edge2 = cross(incoming.direction(), edge2);
        float determinant           = dot(edge1, direction_cross_edge2);
        if (fabs(determinant) < epsilon)
        {
            return false; // ray parallel
        }

        float inverse_determinant = 1.0f / determinant;
        vec3f origin_offset       = incoming.origin() - vertex0;
        float barycentric_u       = inverse_determinant * dot(origin_offset, direction_cross_edge2);
        if (barycentric_u < 0.0f || barycentric_u > 1.0f)
        {
            return false;
        }

        vec3f offset_cross_edge1 = cross(origin_offset, edge1);
        float barycentric_v = inverse_determinant * dot(incoming.direction(), offset_cross_edge1);
        if (barycentric_v < 0.0f || barycentric_u + barycentric_v > 1.0f)
        {
            return false;
        }

        float t = inverse_determinant * dot(edge2, offset_cross_edge1);
        if (!t_range.surrounds(t))
        {
            return false;
        }

        record.t     = t;
        record.point = incoming.at(t);
        record.set_face_normal(incoming, unit_vector(cross(edge1, edge2)));
        record.surface_material = surface_material.get();
        return true;
    }

    AABB bounding_box() const override
    {
        point3f min_point(fmin(vertex0.x(), fmin(vertex1.x(), vertex2.x())),
                          fmin(vertex0.y(), fmin(vertex1.y(), vertex2.y())),
                          fmin(vertex0.z(), fmin(vertex1.z(), vertex2.z())));
        point3f max_point(fmax(vertex0.x(), fmax(vertex1.x(), vertex2.x())),
                          fmax(vertex0.y(), fmax(vertex1.y(), vertex2.y())),
                          fmax(vertex0.z(), fmax(vertex1.z(), vertex2.z())));
        return AABB(min_point, max_point);
    }
};

/*
 * A mesh triangle that also carries texture coordinates and vertex normals, for models that come
 * with them (see load_obj_with_attributes). Both are interpolated across the face, so textured
 * models keep their colors and curved ones shade smoothly. Plain scan meshes keep using the
 * leaner mesh_triangle, whose intersection code is deliberately left as it was; this class
 * repeats the same test because it also needs the barycentric coordinates.
 */
class attributed_triangle : public mesh_triangle
{
public:
    float texture_u[3] = {0, 0, 0}; // texture coordinates at the three vertices
    float texture_v[3] = {0, 0, 0};
    vec3f normals[3]; // outward unit normals at the vertices, used if has_normals
    bool has_normals = false;

    using mesh_triangle::mesh_triangle;

    // Möller–Trumbore intersection
    // ref: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    // On a hit, t is the ray parameter and the hit point is
    // (1 - barycentric_u - barycentric_v) * vertex0 + barycentric_u * vertex1 + barycentric_v *
    // vertex2
    FORCE_INLINE bool intersect(const ray& incoming, interval t_range, float& t,
                                float& barycentric_u, float& barycentric_v) const
    {
        const float epsilon = std::numeric_limits<float>::epsilon();
        vec3f edge1         = vertex1 - vertex0;
        vec3f edge2         = vertex2 - vertex0;

        vec3f direction_cross_edge2 = cross(incoming.direction(), edge2);
        float determinant           = dot(edge1, direction_cross_edge2);
        if (fabs(determinant) < epsilon)
        {
            return false; // ray parallel
        }

        float inverse_determinant = 1.0f / determinant;
        vec3f origin_offset       = incoming.origin() - vertex0;
        barycentric_u             = inverse_determinant * dot(origin_offset, direction_cross_edge2);
        if (barycentric_u < 0.0f || barycentric_u > 1.0f)
        {
            return false;
        }

        vec3f offset_cross_edge1 = cross(origin_offset, edge1);
        barycentric_v = inverse_determinant * dot(incoming.direction(), offset_cross_edge1);
        if (barycentric_v < 0.0f || barycentric_u + barycentric_v > 1.0f)
        {
            return false;
        }

        t = inverse_determinant * dot(edge2, offset_cross_edge1);
        return t_range.surrounds(t);
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        float t, barycentric_u, barycentric_v;
        if (!intersect(incoming, t_range, t, barycentric_u, barycentric_v))
        {
            return false;
        }

        const float weight0 = 1.0f - barycentric_u - barycentric_v;
        const float u =
            weight0 * texture_u[0] + barycentric_u * texture_u[1] + barycentric_v * texture_u[2];
        const float v =
            weight0 * texture_v[0] + barycentric_u * texture_v[1] + barycentric_v * texture_v[2];

        // A cut-out texel is no hit. Checked before writing `record`, which may hold a closer hit.
        if (surface_material->masked_out(u, v))
        {
            return false;
        }

        vec3f outward_normal = unit_vector(cross(vertex1 - vertex0, vertex2 - vertex0));
        if (has_normals)
        {
            outward_normal = unit_vector(weight0 * normals[0] + barycentric_u * normals[1] +
                                         barycentric_v * normals[2]);
        }

        record.t     = t;
        record.point = incoming.at(t);
        record.set_face_normal(incoming, outward_normal);
        record.u                = u;
        record.v                = v;
        record.surface_material = surface_material.get();
        return true;
    }
};

// Reads an OBJ file with its faces triangulated; throws std::runtime_error with the loader's
// message
inline void read_obj(const std::string& filename, tinyobj::attrib_t& attrib,
                     std::vector<tinyobj::shape_t>& shapes)
{
    std::vector<tinyobj::material_t> materials;
    std::string warning, error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filename.c_str()))
    {
        std::string message = error.empty() ? warning : error;
        while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back())))
        {
            message.pop_back(); // tinyobjloader ends its messages with a newline
        }
        throw std::runtime_error("could not load model '" + filename + "': " + message);
    }
}

inline shared_ptr<hittable_list> load_obj(const std::string& filename,
                                          shared_ptr<material> default_material)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    read_obj(filename, attrib, shapes);

    auto mesh = make_shared<hittable_list>();

    for (const auto& shape : shapes)
    {
        size_t index_offset = 0;
        for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); face++)
        {
            int face_vertex_count = shape.mesh.num_face_vertices[face]; // should be 3 for triangles
            assert(face_vertex_count == 3);

            std::array<point3f, 3> vertices;
            for (int vertex = 0; vertex < face_vertex_count; vertex++)
            {
                tinyobj::index_t index = shape.mesh.indices[index_offset + vertex];
                float x                = attrib.vertices[3 * index.vertex_index + 0];
                float y                = attrib.vertices[3 * index.vertex_index + 1];
                float z                = attrib.vertices[3 * index.vertex_index + 2];
                vertices[vertex]       = point3f(x, y, z);
            }
            index_offset += face_vertex_count;

            mesh->add(make_shared<mesh_triangle>(vertices[0], vertices[1], vertices[2],
                                                 default_material));
        }
    }

    return mesh;
}

/*
 * Like load_obj, but keeps the model's texture coordinates and vertex normals (see
 * attributed_triangle): with a texture on the material, the model shows its own colors.
 */
inline shared_ptr<hittable_list> load_obj_with_attributes(const std::string& filename,
                                                          shared_ptr<material> surface_material)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    read_obj(filename, attrib, shapes);

    auto mesh = make_shared<hittable_list>();

    for (const auto& shape : shapes)
    {
        size_t index_offset = 0;
        for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); face++)
        {
            const int face_vertex_count = shape.mesh.num_face_vertices[face];
            assert(face_vertex_count == 3);

            point3f vertices[3];
            float texture_u[3] = {0, 0, 0};
            float texture_v[3] = {0, 0, 0};
            vec3f normals[3];
            bool has_normals = true;
            for (int vertex = 0; vertex < face_vertex_count; vertex++)
            {
                const tinyobj::index_t index = shape.mesh.indices[index_offset + vertex];
                vertices[vertex]             = point3f(attrib.vertices[3 * index.vertex_index + 0],
                                                       attrib.vertices[3 * index.vertex_index + 1],
                                                       attrib.vertices[3 * index.vertex_index + 2]);
                if (index.texcoord_index >= 0)
                {
                    texture_u[vertex] = attrib.texcoords[2 * index.texcoord_index + 0];
                    texture_v[vertex] = attrib.texcoords[2 * index.texcoord_index + 1];
                }
                if (index.normal_index >= 0)
                {
                    normals[vertex] = vec3f(attrib.normals[3 * index.normal_index + 0],
                                            attrib.normals[3 * index.normal_index + 1],
                                            attrib.normals[3 * index.normal_index + 2]);
                }
                else
                {
                    has_normals = false;
                }
            }
            index_offset += face_vertex_count;

            auto triangle = make_shared<attributed_triangle>(vertices[0], vertices[1], vertices[2],
                                                             surface_material);
            for (int vertex = 0; vertex < 3; ++vertex)
            {
                triangle->texture_u[vertex] = texture_u[vertex];
                triangle->texture_v[vertex] = texture_v[vertex];
                triangle->normals[vertex]   = normals[vertex];
            }
            triangle->has_normals = has_normals;
            mesh->add(triangle);
        }
    }

    return mesh;
}

/*
 * Gives every attributed triangle that has no vertex normals smooth ones, computed from the
 * geometry. A vertex's normal is the area-weighted average of the face normals around it that
 * stay within `crease_degrees` of the triangle's own, so curved surfaces shade smoothly and sharp
 * edges stay sharp. Vertices are matched by position.
 */
inline void compute_smooth_normals(hittable_list& mesh, float crease_degrees)
{
    std::vector<attributed_triangle*> faces;
    for (const auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<attributed_triangle>(object);
        if (face && !face->has_normals)
        {
            faces.push_back(face.get());
        }
    }

    // Number the distinct positions, and list the faces around each
    std::map<std::array<float, 3>, int> position_ids;
    std::vector<std::vector<int>> faces_around;
    std::vector<std::array<int, 3>> corner_ids(faces.size());
    std::vector<vec3f> area_normals(faces.size()); // cross products: their length is twice the area
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const point3f* corners[3] = {&faces[i]->vertex0, &faces[i]->vertex1, &faces[i]->vertex2};
        area_normals[i]           = cross(*corners[1] - *corners[0], *corners[2] - *corners[0]);
        for (int corner = 0; corner < 3; ++corner)
        {
            const std::array<float, 3> key = {corners[corner]->x(), corners[corner]->y(),
                                              corners[corner]->z()};
            auto inserted = position_ids.try_emplace(key, static_cast<int>(position_ids.size()));
            if (inserted.second)
            {
                faces_around.emplace_back();
            }
            corner_ids[i][corner] = inserted.first->second;
            faces_around[inserted.first->second].push_back(static_cast<int>(i));
        }
    }

    const float cos_crease = std::cos(degrees_to_radians(crease_degrees));
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const vec3f own_direction = unit_vector(area_normals[i]);
        for (int corner = 0; corner < 3; ++corner)
        {
            vec3f sum(0, 0, 0);
            for (int other : faces_around[corner_ids[i][corner]])
            {
                if (dot(unit_vector(area_normals[other]), own_direction) >= cos_crease)
                {
                    sum += area_normals[other];
                }
            }
            faces[i]->normals[corner] = unit_vector(sum);
        }
        faces[i]->has_normals = true;
    }
}

/*
 * Like load_obj_with_attributes, and a model with no normals of its own gets smooth ones
 * computed from its geometry (see compute_smooth_normals). Models with only a few thousand
 * triangles, like the teapot, look faceted in reflections without it.
 */
inline shared_ptr<hittable_list> load_obj_smooth(const std::string& filename,
                                                 shared_ptr<material> surface_material,
                                                 float crease_degrees = 45.0f)
{
    auto mesh = load_obj_with_attributes(filename, surface_material);
    compute_smooth_normals(*mesh, crease_degrees);
    return mesh;
}

// Uniform scale, then translate
inline void transform_mesh(hittable_list& mesh, float scale, const vec3f& offset)
{
    for (auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<mesh_triangle>(object);
        if (face)
        {
            face->vertex0 = scale * face->vertex0 + offset;
            face->vertex1 = scale * face->vertex1 + offset;
            face->vertex2 = scale * face->vertex2 + offset;
        }
    }
}

// Per-axis scale, then translate
inline void transform_mesh(hittable_list& mesh, const vec3f& scale, const vec3f& offset)
{
    for (auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<mesh_triangle>(object);
        if (face)
        {
            face->vertex0 = scale * face->vertex0 + offset;
            face->vertex1 = scale * face->vertex1 + offset;
            face->vertex2 = scale * face->vertex2 + offset;
        }

        // Normals scale by the inverse, so they stay perpendicular to the stretched surface
        auto attributed = std::dynamic_pointer_cast<attributed_triangle>(object);
        if (attributed && attributed->has_normals)
        {
            for (vec3f& normal : attributed->normals)
            {
                normal = unit_vector(
                    vec3f(normal.x() / scale.x(), normal.y() / scale.y(), normal.z() / scale.z()));
            }
        }
    }
}

/*
 * Rotate every vertex by `degrees` about the coordinate axis `axis` (0 = x, 1 = y, 2 = z),
 * through the origin. Unlike wrapping the mesh in rotate_y, this costs nothing per ray.
 */
inline void rotate_mesh(hittable_list& mesh, int axis, float degrees)
{
    float radians = degrees_to_radians(degrees);
    float sine    = std::sin(radians);
    float cosine  = std::cos(radians);
    int first     = (axis + 1) % 3; // the two coordinates that change, in right-handed order
    int second    = (axis + 2) % 3;

    auto rotate = [&](point3f& point)
    {
        float rotated_first  = cosine * point[first] - sine * point[second];
        float rotated_second = sine * point[first] + cosine * point[second];
        point[first]         = rotated_first;
        point[second]        = rotated_second;
    };

    for (auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<mesh_triangle>(object);
        if (face)
        {
            rotate(face->vertex0);
            rotate(face->vertex1);
            rotate(face->vertex2);
        }

        auto attributed = std::dynamic_pointer_cast<attributed_triangle>(object);
        if (attributed)
        {
            for (vec3f& normal : attributed->normals)
            {
                rotate(normal);
            }
        }
    }
}

/*
 * Scales the mesh uniformly to `height` along y and moves it so the middle of its base sits at
 * `base_center`, standing models of any units and position on the floor.
 */
inline void place_mesh(hittable_list& mesh, float height, const point3f& base_center)
{
    point3f min_corner(INF, INF, INF);
    point3f max_corner(-INF, -INF, -INF);
    for (const auto& object : mesh.objects)
    {
        auto face = std::dynamic_pointer_cast<mesh_triangle>(object);
        if (face)
        {
            for (const point3f* vertex : {&face->vertex0, &face->vertex1, &face->vertex2})
            {
                for (int axis = 0; axis < 3; ++axis)
                {
                    min_corner[axis] = std::fmin(min_corner[axis], (*vertex)[axis]);
                    max_corner[axis] = std::fmax(max_corner[axis], (*vertex)[axis]);
                }
            }
        }
    }

    float scale = height / (max_corner.y() - min_corner.y());
    point3f current_base(0.5f * (min_corner.x() + max_corner.x()), min_corner.y(),
                         0.5f * (min_corner.z() + max_corner.z()));
    transform_mesh(mesh, scale, base_center - scale * current_base);
}

} // namespace rt
