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

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        if (nodes.empty())
            return false;

        const point3f& o   = r.origin();
        const float inv[3] = {1.0f / r.direction().x(), 1.0f / r.direction().y(),
                              1.0f / r.direction().z()};

        float t_enter;
        if (!nodes[0].hit(o, inv, ray_t.min, ray_t.max, t_enter))
            return false;

        struct Entry
        {
            uint32_t node;
            float t_enter;
        };

        Entry stack[128]; // SAH trees stay far shallower than this
        int sp            = 0;
        uint32_t current  = 0;
        bool hit_anything = false;

        while (true)
        {
            const Node& node = nodes[current];

            if (node.count > 0)
            {
                for (uint32_t i = node.first; i < node.first + node.count; ++i)
                {
                    if (prims[i]->hit(r, ray_t, rec))
                    {
                        hit_anything = true;
                        ray_t.max    = rec.t;
                    }
                }
            }
            else
            {
                // Visit the nearer child first; push the farther one if it is hit at all.
                uint32_t a = node.first, b = node.first + 1;
                float ta, tb;
                bool hit_a = nodes[a].hit(o, inv, ray_t.min, ray_t.max, ta);
                bool hit_b = nodes[b].hit(o, inv, ray_t.min, ray_t.max, tb);

                if (hit_a && hit_b)
                {
                    if (tb < ta)
                    {
                        std::swap(a, b);
                        std::swap(ta, tb);
                    }
                    stack[sp++] = {b, tb};
                    current     = a;
                    continue;
                }
                if (hit_a)
                {
                    current = a;
                    continue;
                }
                if (hit_b)
                {
                    current = b;
                    continue;
                }
            }

            // Pop the next node that still starts in front of the closest hit so far.
            do
            {
                if (sp == 0)
                    return hit_anything;
                --sp;
            } while (stack[sp].t_enter > ray_t.max);
            current = stack[sp].node;
        }
    }

    AABB bounding_box() const override { return bbox; }

private:
    struct Node
    {
        float bmin[3], bmax[3];
        uint32_t first; // interior: index of left child; leaf: index of first primitive
        uint32_t count; // 0 for interior nodes

        // Slab test against [t_min, t_max]; on a hit, t_enter is where the ray enters the box
        bool hit(const point3f& o, const float inv[3], float t_min, float t_max,
                 float& t_enter) const
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                float t0 = (bmin[axis] - o[axis]) * inv[axis];
                float t1 = (bmax[axis] - o[axis]) * inv[axis];
                if (inv[axis] < 0.0f)
                    std::swap(t0, t1);
                t_min = t0 > t_min ? t0 : t_min;
                t_max = t1 < t_max ? t1 : t_max;
                if (t_max <= t_min)
                    return false;
            }
            t_enter = t_min;
            return true;
        }
    };

    struct Bounds
    {
        float bmin[3] = {INF, INF, INF};
        float bmax[3] = {-INF, -INF, -INF};

        void grow(const float lo[3], const float hi[3])
        {
            for (int a = 0; a < 3; ++a)
            {
                bmin[a] = std::min(bmin[a], lo[a]);
                bmax[a] = std::max(bmax[a], hi[a]);
            }
        }

        void grow(const float p[3]) { grow(p, p); }

        float area() const
        {
            float dx = bmax[0] - bmin[0], dy = bmax[1] - bmin[1], dz = bmax[2] - bmin[2];
            if (dx < 0 || dy < 0 || dz < 0)
                return 0.0f; // empty
            return 2.0f * (dx * dy + dy * dz + dz * dx);
        }
    };

    struct BuildPrim
    {
        float bmin[3], bmax[3], centroid[3];
    };

    static constexpr int BINS          = 12;
    static constexpr uint32_t MAX_LEAF = 4;

    std::vector<shared_ptr<hittable>> owned; // keeps the primitives alive, in leaf order
    std::vector<const hittable*> prims;      // raw pointers for traversal, same order
    std::vector<Node> nodes;
    AABB bbox;

    void build()
    {
        if (owned.empty())
            return;

        std::vector<BuildPrim> info(owned.size());
        for (size_t i = 0; i < owned.size(); ++i)
        {
            AABB b                  = owned[i]->bounding_box();
            const interval* axes[3] = {&b.x, &b.y, &b.z};
            for (int a = 0; a < 3; ++a)
            {
                info[i].bmin[a]     = axes[a]->min;
                info[i].bmax[a]     = axes[a]->max;
                info[i].centroid[a] = 0.5f * (axes[a]->min + axes[a]->max);
            }
            bbox = AABB(bbox, b);
        }

        std::vector<uint32_t> order(owned.size());
        for (uint32_t i = 0; i < order.size(); ++i)
            order[i] = i;

        nodes.reserve(2 * owned.size());
        nodes.push_back({});
        subdivide(0, 0, static_cast<uint32_t>(order.size()), info, order);

        // Reorder primitives to match the leaves
        std::vector<shared_ptr<hittable>> sorted(owned.size());
        prims.resize(owned.size());
        for (size_t i = 0; i < order.size(); ++i)
        {
            sorted[i] = owned[order[i]];
            prims[i]  = sorted[i].get();
        }
        owned = std::move(sorted);
    }

    void subdivide(uint32_t node_index, uint32_t first, uint32_t count,
                   const std::vector<BuildPrim>& info, std::vector<uint32_t>& order)
    {
        Bounds bounds, centroids;
        for (uint32_t i = first; i < first + count; ++i)
        {
            bounds.grow(info[order[i]].bmin, info[order[i]].bmax);
            centroids.grow(info[order[i]].centroid);
        }

        Node& node = nodes[node_index];
        for (int a = 0; a < 3; ++a)
        {
            node.bmin[a] = bounds.bmin[a];
            node.bmax[a] = bounds.bmax[a];
        }
        node.first = first;
        node.count = count;
        if (count <= 1)
            return;

        // Find the cheapest SAH split over all three axes
        int best_axis    = -1;
        float best_split = 0.0f;
        float best_cost  = INF;

        for (int axis = 0; axis < 3; ++axis)
        {
            float lo = centroids.bmin[axis], hi = centroids.bmax[axis];
            if (hi - lo <= 0.0f)
                continue; // all centroids coincide on this axis
            float scale = BINS / (hi - lo);

            Bounds bin_bounds[BINS];
            uint32_t bin_count[BINS] = {};
            for (uint32_t i = first; i < first + count; ++i)
            {
                const BuildPrim& p = info[order[i]];
                int b = std::min(BINS - 1, static_cast<int>((p.centroid[axis] - lo) * scale));
                bin_count[b]++;
                bin_bounds[b].grow(p.bmin, p.bmax);
            }

            // Sweep from both sides to get the area/count left and right of each plane
            float left_area[BINS - 1], right_area[BINS - 1];
            uint32_t left_count[BINS - 1], right_count[BINS - 1];
            Bounds left, right;
            uint32_t left_sum = 0, right_sum = 0;
            for (int i = 0; i < BINS - 1; ++i)
            {
                left_sum += bin_count[i];
                left.grow(bin_bounds[i].bmin, bin_bounds[i].bmax);
                left_count[i] = left_sum;
                left_area[i]  = left.area();

                right_sum += bin_count[BINS - 1 - i];
                right.grow(bin_bounds[BINS - 1 - i].bmin, bin_bounds[BINS - 1 - i].bmax);
                right_count[BINS - 2 - i] = right_sum;
                right_area[BINS - 2 - i]  = right.area();
            }

            for (int i = 0; i < BINS - 1; ++i)
            {
                if (left_count[i] == 0 || right_count[i] == 0)
                    continue;
                float cost = left_count[i] * left_area[i] + right_count[i] * right_area[i];
                if (cost < best_cost)
                {
                    best_cost  = cost;
                    best_axis  = axis;
                    best_split = lo + (i + 1) / scale;
                }
            }
        }

        if (best_axis < 0)
        {
            // All centroids coincide: no plane separates them, so just halve big lists
            if (count > MAX_LEAF)
                split_children(node_index, first, count, count / 2, info, order);
            return;
        }

        // Small enough and splitting doesn't pay off (traversal cost ~ one intersection): leaf
        float leaf_cost  = count * bounds.area();
        float split_cost = bounds.area() + best_cost;
        if (count <= MAX_LEAF && split_cost >= leaf_cost)
            return;

        auto mid_it =
            std::partition(order.begin() + first, order.begin() + first + count,
                           [&](uint32_t i) { return info[i].centroid[best_axis] < best_split; });
        uint32_t left_count = static_cast<uint32_t>(mid_it - (order.begin() + first));
        if (left_count == 0 || left_count == count)
            left_count = count / 2; // float edge case

        split_children(node_index, first, count, left_count, info, order);
    }

    void split_children(uint32_t node_index, uint32_t first, uint32_t count, uint32_t left_count,
                        const std::vector<BuildPrim>& info, std::vector<uint32_t>& order)
    {
        uint32_t left_index = static_cast<uint32_t>(nodes.size());
        nodes.push_back({});
        nodes.push_back({});
        nodes[node_index].first =
            left_index; // nodes may have reallocated: index, don't hold a reference
        nodes[node_index].count = 0;

        subdivide(left_index, first, left_count, info, order);
        subdivide(left_index + 1, first + left_count, count - left_count, info, order);
    }
};

} // namespace rt
