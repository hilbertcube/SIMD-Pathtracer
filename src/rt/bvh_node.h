#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "AABB.h"
#include "hittable.h"
#include "hittable_list.h"

#include "extra/def.h"

namespace rt
{

/**
 * Bounding volume hierarchy over a set of hittables.
 *
 * The tree is stored flat: nodes live in one array, children of an interior node are
 * adjacent (left = first, right = first + 1), and leaves index a contiguous run of
 * primitives. Splits are chosen with the surface area heuristic (SAH) over binned
 * centroids, and traversal is iterative, visiting the nearer child first.
 */
class bvh_node : public hittable
{
public:
    explicit bvh_node(const hittable_list& list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(const std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end)
        : owned(objects.begin() + start, objects.begin() + end)
    {
        build();
    }

    bool hit(const ray& incoming, interval t_range, hit_record& record) const override
    {
        if (nodes.empty())
        {
            return false;
        }

        const point3f& origin            = incoming.origin();
        const float inverse_direction[3] = {1.0f / incoming.direction().x(),
                                            1.0f / incoming.direction().y(),
                                            1.0f / incoming.direction().z()};

        float t_enter;
        if (!nodes[0].hit(origin, inverse_direction, t_range.min, t_range.max, t_enter))
        {
            return false;
        }

        struct StackEntry
        {
            uint32_t node;
            float t_enter;
        };

        StackEntry stack[128]; // SAH trees stay far shallower than this
        int stack_size    = 0;
        uint32_t current  = 0;
        bool hit_anything = false;

        while (true)
        {
            const Node& node = nodes[current];

            if (node.count > 0)
            {
                for (uint32_t i = node.first; i < node.first + node.count; ++i)
                {
                    if (primitives[i]->hit(incoming, t_range, record))
                    {
                        hit_anything = true;
                        t_range.max  = record.t;
                    }
                }
            }
            else
            {
                // Visit the nearer child first; push the farther one if it is hit at all.
                uint32_t first_child = node.first, second_child = node.first + 1;
                float first_t, second_t;
                bool hits_first  = nodes[first_child].hit(origin, inverse_direction, t_range.min,
                                                          t_range.max, first_t);
                bool hits_second = nodes[second_child].hit(origin, inverse_direction, t_range.min,
                                                           t_range.max, second_t);

                if (hits_first && hits_second)
                {
                    if (second_t < first_t)
                    {
                        std::swap(first_child, second_child);
                        std::swap(first_t, second_t);
                    }
                    stack[stack_size++] = {second_child, second_t};
                    current             = first_child;
                    continue;
                }
                if (hits_first)
                {
                    current = first_child;
                    continue;
                }
                if (hits_second)
                {
                    current = second_child;
                    continue;
                }
            }

            // Pop the next node that still starts in front of the closest hit so far.
            do
            {
                if (stack_size == 0)
                {
                    return hit_anything;
                }
                --stack_size;
            } while (stack[stack_size].t_enter > t_range.max);
            current = stack[stack_size].node;
        }
    }

    AABB bounding_box() const override { return bounds; }

private:
    struct Node
    {
        float min_corner[3], max_corner[3];
        uint32_t first; // interior: index of left child; leaf: index of first primitive
        uint32_t count; // 0 for interior nodes

        // Slab test against [t_min, t_max]; on a hit, t_enter is where the ray enters the box
        bool hit(const point3f& origin, const float inverse_direction[3], float t_min, float t_max,
                 float& t_enter) const
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                float t_near = (min_corner[axis] - origin[axis]) * inverse_direction[axis];
                float t_far  = (max_corner[axis] - origin[axis]) * inverse_direction[axis];
                if (inverse_direction[axis] < 0.0f)
                {
                    std::swap(t_near, t_far);
                }
                t_min = t_near > t_min ? t_near : t_min;
                t_max = t_far < t_max ? t_far : t_max;
                if (t_max <= t_min)
                {
                    return false;
                }
            }
            t_enter = t_min;
            return true;
        }
    };

    struct Bounds
    {
        float min_corner[3] = {INF, INF, INF};
        float max_corner[3] = {-INF, -INF, -INF};

        void grow(const float low[3], const float high[3])
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                min_corner[axis] = std::min(min_corner[axis], low[axis]);
                max_corner[axis] = std::max(max_corner[axis], high[axis]);
            }
        }

        void grow(const float point[3]) { grow(point, point); }

        float area() const
        {
            float size_x = max_corner[0] - min_corner[0], size_y = max_corner[1] - min_corner[1],
                  size_z = max_corner[2] - min_corner[2];
            if (size_x < 0 || size_y < 0 || size_z < 0)
            {
                return 0.0f; // empty
            }
            return 2.0f * (size_x * size_y + size_y * size_z + size_z * size_x);
        }
    };

    struct BuildPrimitive
    {
        float min_corner[3], max_corner[3], centroid[3];
    };

    static constexpr int BIN_COUNT          = 12;
    static constexpr uint32_t MAX_LEAF_SIZE = 4;

    std::vector<shared_ptr<hittable>> owned; // keeps the primitives alive, in leaf order
    std::vector<const hittable*> primitives; // raw pointers for traversal, same order
    std::vector<Node> nodes;
    AABB bounds;

    void build()
    {
        if (owned.empty())
        {
            return;
        }

        std::vector<BuildPrimitive> primitive_info(owned.size());
        for (size_t i = 0; i < owned.size(); ++i)
        {
            AABB object_bounds      = owned[i]->bounding_box();
            const interval* axes[3] = {&object_bounds.x, &object_bounds.y, &object_bounds.z};
            for (int axis = 0; axis < 3; ++axis)
            {
                primitive_info[i].min_corner[axis] = axes[axis]->min;
                primitive_info[i].max_corner[axis] = axes[axis]->max;
                primitive_info[i].centroid[axis]   = 0.5f * (axes[axis]->min + axes[axis]->max);
            }
            bounds = AABB(bounds, object_bounds);
        }

        std::vector<uint32_t> order(owned.size());
        for (uint32_t i = 0; i < order.size(); ++i)
        {
            order[i] = i;
        }

        nodes.reserve(2 * owned.size());
        nodes.push_back({});
        subdivide(0, 0, static_cast<uint32_t>(order.size()), primitive_info, order);

        // Reorder primitives to match the leaves
        std::vector<shared_ptr<hittable>> sorted(owned.size());
        primitives.resize(owned.size());
        for (size_t i = 0; i < order.size(); ++i)
        {
            sorted[i]     = owned[order[i]];
            primitives[i] = sorted[i].get();
        }
        owned = std::move(sorted);
    }

    void subdivide(uint32_t node_index, uint32_t first, uint32_t count,
                   const std::vector<BuildPrimitive>& primitive_info, std::vector<uint32_t>& order)
    {
        Bounds node_bounds, centroids;
        for (uint32_t i = first; i < first + count; ++i)
        {
            node_bounds.grow(primitive_info[order[i]].min_corner,
                             primitive_info[order[i]].max_corner);
            centroids.grow(primitive_info[order[i]].centroid);
        }

        Node& node = nodes[node_index];
        for (int axis = 0; axis < 3; ++axis)
        {
            node.min_corner[axis] = node_bounds.min_corner[axis];
            node.max_corner[axis] = node_bounds.max_corner[axis];
        }
        node.first = first;
        node.count = count;
        if (count <= 1)
        {
            return;
        }

        // Find the cheapest SAH split over all three axes
        int best_axis    = -1;
        float best_split = 0.0f;
        float best_cost  = INF;

        for (int axis = 0; axis < 3; ++axis)
        {
            float low = centroids.min_corner[axis], high = centroids.max_corner[axis];
            if (high - low <= 0.0f)
            {
                continue; // all centroids coincide on this axis
            }
            float scale = BIN_COUNT / (high - low);

            Bounds bin_bounds[BIN_COUNT];
            uint32_t bin_count[BIN_COUNT] = {};
            for (uint32_t i = first; i < first + count; ++i)
            {
                const BuildPrimitive& primitive = primitive_info[order[i]];
                int bin                         = std::min(BIN_COUNT - 1,
                                                           static_cast<int>((primitive.centroid[axis] - low) * scale));
                bin_count[bin]++;
                bin_bounds[bin].grow(primitive.min_corner, primitive.max_corner);
            }

            // Sweep from both sides to get the area/count left and right of each plane
            float left_area[BIN_COUNT - 1], right_area[BIN_COUNT - 1];
            uint32_t left_count[BIN_COUNT - 1], right_count[BIN_COUNT - 1];
            Bounds left, right;
            uint32_t left_sum = 0, right_sum = 0;
            for (int i = 0; i < BIN_COUNT - 1; ++i)
            {
                left_sum += bin_count[i];
                left.grow(bin_bounds[i].min_corner, bin_bounds[i].max_corner);
                left_count[i] = left_sum;
                left_area[i]  = left.area();

                right_sum += bin_count[BIN_COUNT - 1 - i];
                right.grow(bin_bounds[BIN_COUNT - 1 - i].min_corner,
                           bin_bounds[BIN_COUNT - 1 - i].max_corner);
                right_count[BIN_COUNT - 2 - i] = right_sum;
                right_area[BIN_COUNT - 2 - i]  = right.area();
            }

            for (int i = 0; i < BIN_COUNT - 1; ++i)
            {
                if (left_count[i] == 0 || right_count[i] == 0)
                {
                    continue;
                }
                float cost = left_count[i] * left_area[i] + right_count[i] * right_area[i];
                if (cost < best_cost)
                {
                    best_cost  = cost;
                    best_axis  = axis;
                    best_split = low + (i + 1) / scale;
                }
            }
        }

        if (best_axis < 0)
        {
            // All centroids coincide: no plane separates them, so just halve big lists
            if (count > MAX_LEAF_SIZE)
            {
                split_children(node_index, first, count, count / 2, primitive_info, order);
            }
            return;
        }

        // Small enough and splitting doesn't pay off (traversal cost ~ one intersection): leaf
        float leaf_cost  = count * node_bounds.area();
        float split_cost = node_bounds.area() + best_cost;
        if (count <= MAX_LEAF_SIZE && split_cost >= leaf_cost)
        {
            return;
        }

        auto middle =
            std::partition(order.begin() + first, order.begin() + first + count, [&](uint32_t i)
                           { return primitive_info[i].centroid[best_axis] < best_split; });
        uint32_t left_count = static_cast<uint32_t>(middle - (order.begin() + first));
        if (left_count == 0 || left_count == count)
        {
            left_count = count / 2; // float edge case
        }

        split_children(node_index, first, count, left_count, primitive_info, order);
    }

    void split_children(uint32_t node_index, uint32_t first, uint32_t count, uint32_t left_count,
                        const std::vector<BuildPrimitive>& primitive_info,
                        std::vector<uint32_t>& order)
    {
        uint32_t left_index = static_cast<uint32_t>(nodes.size());
        nodes.push_back({});
        nodes.push_back({});
        nodes[node_index].first =
            left_index; // nodes may have reallocated: index, don't hold a reference
        nodes[node_index].count = 0;

        subdivide(left_index, first, left_count, primitive_info, order);
        subdivide(left_index + 1, first + left_count, count - left_count, primitive_info, order);
    }
};

} // namespace rt
