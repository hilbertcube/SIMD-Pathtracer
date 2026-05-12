#pragma once

#include "rt/geometry/hittable_list.h"
#include <algorithm>

#include "rt/math/vector.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

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

shared_ptr<hittable_list> load_obj(const std::string& filename,
                                   shared_ptr<material> default_material)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
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

// allow shear
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

// only scale size
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
    }
}

} // namespace rt
