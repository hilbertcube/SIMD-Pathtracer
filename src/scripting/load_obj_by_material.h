#pragma once

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "rt/geometry/mesh.h"
#include "rt/materials/materials.h"

namespace scripting
{

using material_picker = std::function<rt::shared_ptr<rt::material>(const std::string&)>;

/*
 * Load an OBJ file as triangles that each get the material `pick` returns for the face's usemtl
 * name. The file's own .mtl is ignored, so the scene decides how every part of a model looks.
 * Texture coordinates and vertex normals are kept; polygons are split into triangle fans.
 */
inline rt::shared_ptr<rt::hittable_list> load_obj_by_material(const std::string& filename,
                                                              const material_picker& pick)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("could not open " + filename);
    }

    std::vector<rt::point3f> positions;
    std::vector<rt::vec3f> normals;
    std::vector<std::pair<float, float>> texture_coordinates;
    std::unordered_map<std::string, rt::shared_ptr<rt::material>> materials;
    rt::shared_ptr<rt::material> current = pick("");
    auto mesh                            = rt::make_shared<rt::hittable_list>();

    // An OBJ index counts from 1, or from the end of the list if negative
    auto resolve = [](long index, size_t count) -> long
    {
        return index > 0 ? index - 1 : static_cast<long>(count) + index;
    };

    std::string line;
    while (std::getline(file, line))
    {
        float x = 0, y = 0, z = 0;
        if (line.compare(0, 2, "v ") == 0)
        {
            std::sscanf(line.c_str() + 2, "%f %f %f", &x, &y, &z);
            positions.emplace_back(x, y, z);
        }
        else if (line.compare(0, 3, "vn ") == 0)
        {
            std::sscanf(line.c_str() + 3, "%f %f %f", &x, &y, &z);
            normals.push_back(rt::unit_vector(rt::vec3f(x, y, z)));
        }
        else if (line.compare(0, 3, "vt ") == 0)
        {
            std::sscanf(line.c_str() + 3, "%f %f", &x, &y);
            texture_coordinates.emplace_back(x, y);
        }
        else if (line.compare(0, 7, "usemtl ") == 0)
        {
            std::string name = line.substr(7);
            while (!name.empty() && (name.back() == '\r' || name.back() == ' '))
            {
                name.pop_back();
            }
            auto found = materials.find(name);
            if (found == materials.end())
            {
                found = materials.emplace(name, pick(name)).first;
            }
            current = found->second;
        }
        else if (line.compare(0, 2, "f ") == 0)
        {
            // Each corner is "position", "position/texture", "position//normal" or all three
            long corners[16][3];
            int corner_count = 0;
            const char* text = line.c_str() + 2;
            while (*text && corner_count < 16)
            {
                while (*text == ' ')
                {
                    ++text;
                }
                if (!*text || *text == '\r')
                {
                    break;
                }
                long indices[3] = {0, 0, 0};
                char* end;
                indices[0] = std::strtol(text, &end, 10);
                text       = end;
                for (int part = 1; part < 3 && *text == '/'; ++part)
                {
                    ++text;
                    if (*text != '/' && *text != ' ' && *text != '\r' && *text)
                    {
                        indices[part] = std::strtol(text, &end, 10);
                        text          = end;
                    }
                }
                for (int part = 0; part < 3; ++part)
                {
                    corners[corner_count][part] = indices[part];
                }
                ++corner_count;
            }

            for (int k = 1; k + 1 < corner_count; ++k)
            {
                const int order[3] = {0, k, k + 1};
                rt::point3f points[3];
                for (int i = 0; i < 3; ++i)
                {
                    points[i] = positions[resolve(corners[order[i]][0], positions.size())];
                }
                auto triangle = rt::make_shared<rt::attributed_triangle>(points[0], points[1],
                                                                         points[2], current);

                bool all_normals = true;
                for (int i = 0; i < 3; ++i)
                {
                    const long* corner = corners[order[i]];
                    if (corner[1] != 0)
                    {
                        const auto& uv =
                            texture_coordinates[resolve(corner[1], texture_coordinates.size())];
                        triangle->texture_u[i] = uv.first;
                        triangle->texture_v[i] = uv.second;
                    }
                    if (corner[2] != 0)
                    {
                        triangle->normals[i] = normals[resolve(corner[2], normals.size())];
                    }
                    else
                    {
                        all_normals = false;
                    }
                }
                triangle->has_normals = all_normals;
                mesh->add(triangle);
            }
        }
    }
    return mesh;
}

} // namespace scripting
