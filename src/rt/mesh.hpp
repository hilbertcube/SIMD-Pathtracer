#pragma once

#include "hittable_list.hpp"
#include <algorithm>

#include "rtm/vector.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

namespace rt {

class mesh_triangle : public hittable {
public:
    point3f v0, v1, v2;
    shared_ptr<material> mat;

    mesh_triangle() {}
    mesh_triangle(const point3f& a, const point3f& b, const point3f& c, shared_ptr<material> m)
        : v0(a), v1(b), v2(c), mat(m) {}

    // Möller–Trumbore intersection
    // ref: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const float epsilon = std::numeric_limits<float>::epsilon();
        vec3f edge1 = v1 - v0;
        vec3f edge2 = v2 - v0;

        vec3f h = cross(r.direction(), edge2);
        float a = dot(edge1, h);
        if (fabs(a) < epsilon) return false; // ray parallel

        float f = 1.0f / a;
        vec3f s = r.origin() - v0;
        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;

        vec3f q = cross(s, edge1);
        float v = f * dot(r.direction(), q);
        if (v < 0.0f || u + v > 1.0f) return false;

        float t = f * dot(edge2, q);
        if (!ray_t.surrounds(t)) return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, unit_vector(cross(edge1, edge2)));
        rec.mat = mat;
        return true;
    }

    AABB bounding_box() const override {
        point3f min_pt(fmin(v0.x(), fmin(v1.x(), v2.x())),
                      fmin(v0.y(), fmin(v1.y(), v2.y())),
                      fmin(v0.z(), fmin(v1.z(), v2.z())));
        point3f max_pt(fmax(v0.x(), fmax(v1.x(), v2.x())),
                      fmax(v0.y(), fmax(v1.y(), v2.y())),
                      fmax(v0.z(), fmax(v1.z(), v2.z())));
        return AABB(min_pt, max_pt);
    }
};

shared_ptr<hittable_list> load_obj(
    const std::string& filename,
    shared_ptr<material> default_mat)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        throw std::runtime_error(warn + err);
    }

    auto mesh = make_shared<hittable_list>();

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f]; // should be 3 for triangles
            assert(fv == 3);

            std::array<point3f, 3> verts;
            for (int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                verts[v] = point3f(vx, vy, vz);
            }
            index_offset += fv;

            mesh->add(make_shared<mesh_triangle>(verts[0], verts[1], verts[2], default_mat));
        }
    }

    return mesh;
}

// allow shear
inline void transform_mesh(hittable_list& mesh, float scale, const vec3f& translate) {
    for (auto& obj : mesh.objects) {
        auto tri = std::dynamic_pointer_cast<mesh_triangle>(obj);
        if (tri) {
            tri->v0 = scale * tri->v0 + translate;
            tri->v1 = scale * tri->v1 + translate;
            tri->v2 = scale * tri->v2 + translate;
        }
    }
}

// only scale size
inline void transform_mesh(hittable_list& mesh, const vec3f& scale, const vec3f& translate) {
    for (auto& obj : mesh.objects) {
        auto tri = std::dynamic_pointer_cast<mesh_triangle>(obj);
        if (tri) {
            tri->v0 = scale * tri->v0 + translate;
            tri->v1 = scale * tri->v1 + translate;
            tri->v2 = scale * tri->v2 + translate;
        }
    }
}

} // namespace rt