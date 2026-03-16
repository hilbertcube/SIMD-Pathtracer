#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#if !defined(__SSE2__)
#error "bvh_node.h uses SSE intrinsics and needs an x86-64 target"
#endif
#include <immintrin.h>

#include "AABB.h"
#include "hittable.h"
#include "hittable_list.h"

#include "extra/def.h"

namespace rt
{

/**
 * Bounding volume hierarchy over a set of hittables.
 *
 * The tree is built as a binary tree, choosing splits with the surface area heuristic (SAH)
 * over binned centroids, and then collapsed into a 4-wide tree: every node holds up to four
 * children, and one ray is tested against all four boxes at once with SSE. Nodes live in one
 * flat array, leaves index a contiguous run of primitives, and traversal is iterative,
 * visiting the nearest hit child first.
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

        // Ray data broadcast to all four lanes
        const point3f& origin  = incoming.origin();
        const vec3f& direction = incoming.direction();
        const __m128 origin_x  = _mm_set1_ps(origin.x());
        const __m128 origin_y  = _mm_set1_ps(origin.y());
        const __m128 origin_z  = _mm_set1_ps(origin.z());
        const __m128 inverse_x = _mm_set1_ps(1.0f / direction.x());
        const __m128 inverse_y = _mm_set1_ps(1.0f / direction.y());
        const __m128 inverse_z = _mm_set1_ps(1.0f / direction.z());

        /*
         * The slab the ray enters first on each axis: the minimum side when the ray travels
         * toward +axis, the maximum side otherwise. Choosing it once per ray saves a min/max
         * per axis in every node test.
         */
        const int near_x = direction.x() < 0.0f ? 1 : 0;
        const int near_y = direction.y() < 0.0f ? 1 : 0;
        const int near_z = direction.z() < 0.0f ? 1 : 0;

        StackEntry stack[STACK_SIZE];
        int stack_size     = 0;
        StackEntry current = {0, 0, t_range.min}; // the root node
        bool hit_anything  = false;

        while (true)
        {
            if (current.count > 0)
            {
                // Leaf: test its primitives, shrinking the range to the closest hit
                for (uint32_t i = current.index; i < current.index + current.count; ++i)
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
                // Interior: slab-test all four children at once
                const Node& node = nodes[current.index];
                __m128 t_enter   = _mm_set1_ps(t_range.min);
                __m128 t_exit    = _mm_set1_ps(t_range.max);

                /*
                 * _mm_max_ps / _mm_min_ps return their second operand when either one is NaN
                 * (0 * inf, for a ray parallel to a slab that starts on it). Passing the
                 * running interval second keeps it unchanged in that case, matching the
                 * scalar test this replaces.
                 */
                __m128 t_near = _mm_mul_ps(
                    _mm_sub_ps(_mm_load_ps(node.bounds[near_x][0]), origin_x), inverse_x);
                __m128 t_far = _mm_mul_ps(
                    _mm_sub_ps(_mm_load_ps(node.bounds[1 - near_x][0]), origin_x), inverse_x);
                t_enter = _mm_max_ps(t_near, t_enter);
                t_exit  = _mm_min_ps(t_far, t_exit);

                t_near  = _mm_mul_ps(_mm_sub_ps(_mm_load_ps(node.bounds[near_y][1]), origin_y),
                                     inverse_y);
                t_far   = _mm_mul_ps(_mm_sub_ps(_mm_load_ps(node.bounds[1 - near_y][1]), origin_y),
                                     inverse_y);
                t_enter = _mm_max_ps(t_near, t_enter);
                t_exit  = _mm_min_ps(t_far, t_exit);

                t_near  = _mm_mul_ps(_mm_sub_ps(_mm_load_ps(node.bounds[near_z][2]), origin_z),
                                     inverse_z);
                t_far   = _mm_mul_ps(_mm_sub_ps(_mm_load_ps(node.bounds[1 - near_z][2]), origin_z),
                                     inverse_z);
                t_enter = _mm_max_ps(t_near, t_enter);
                t_exit  = _mm_min_ps(t_far, t_exit);

                int hit_mask = _mm_movemask_ps(_mm_cmplt_ps(t_enter, t_exit));

                if (hit_mask != 0)
                {
                    alignas(16) float enter_distance[4];
                    _mm_store_ps(enter_distance, t_enter);

                    // Collect the children that were hit, sorted nearest first
                    StackEntry hits[4];
                    int hit_count = 0;
                    while (hit_mask != 0)
                    {
                        int lane = __builtin_ctz(hit_mask);
                        hit_mask &= hit_mask - 1;
                        if (node.child[lane] == EMPTY_SLOT)
                        {
                            continue;
                        }

                        StackEntry entry = {node.child[lane], node.count[lane],
                                            enter_distance[lane]};
                        int position     = hit_count++;
                        while (position > 0 && hits[position - 1].t_enter > entry.t_enter)
                        {
                            hits[position] = hits[position - 1];
                            --position;
                        }
                        hits[position] = entry;
                    }

                    if (hit_count > 0)
                    {
                        // Visit the nearest now; push the rest so the next nearest pops first
                        for (int i = hit_count - 1; i > 0; --i)
                        {
                            stack[stack_size++] = hits[i];
                        }
                        current = hits[0];
                        continue;
                    }
                }
            }

            // Pop the next node that still starts in front of the closest hit so far
            do
            {
                if (stack_size == 0)
                {
                    return hit_anything;
                }
                --stack_size;
            } while (stack[stack_size].t_enter > t_range.max);
            current = stack[stack_size];
        }
    }

    AABB bounding_box() const override { return bounds; }

private:
    /*
     * One node of the 4-wide tree. Each of the four lanes describes one child; the bounds are
     * stored structure-of-arrays (all four children's min x, then all four min y, ...) so a
     * single SSE load fetches one slab for every child.
     */
    struct alignas(64) Node
    {
        float bounds[2][3][4]; // [min, max][axis][child]
        uint32_t child[4];     // interior child: node index; leaf child: first primitive
        uint32_t count[4];     // leaf child: number of primitives; interior child: 0
    };

    // Node of the binary SAH tree, built first and then collapsed into Nodes
    struct BinaryNode
    {
        float min_corner[3], max_corner[3];
        uint32_t first; // interior: index of left child; leaf: index of first primitive
        uint32_t count; // 0 for interior nodes

        float area() const
        {
            float size_x = max_corner[0] - min_corner[0];
            float size_y = max_corner[1] - min_corner[1];
            float size_z = max_corner[2] - min_corner[2];
            return 2.0f * (size_x * size_y + size_y * size_z + size_z * size_x);
        }
    };

    struct StackEntry
    {
        uint32_t index; // node index, or first primitive for a leaf
        uint32_t count; // leaf primitive count; 0 for a node
        float t_enter;  // where the ray enters its box
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
    static constexpr uint32_t EMPTY_SLOT    = UINT32_MAX; // lane with no child
    static constexpr int STACK_SIZE         = 128;        // SAH trees stay far shallower

    std::vector<shared_ptr<hittable>> owned; // keeps the primitives alive, in leaf order
    std::vector<const hittable*> primitives; // raw pointers for traversal, same order
    std::vector<Node> nodes;                 // 4-wide tree; nodes[0] is the root
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

        std::vector<BinaryNode> binary_nodes;
        binary_nodes.reserve(2 * owned.size());
        binary_nodes.push_back({});
        subdivide(binary_nodes, 0, 0, static_cast<uint32_t>(order.size()), primitive_info, order);

        nodes.reserve(binary_nodes.size() / 2 + 1);
        collapse(binary_nodes, 0);

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

    void subdivide(std::vector<BinaryNode>& binary_nodes, uint32_t node_index, uint32_t first,
                   uint32_t count, const std::vector<BuildPrimitive>& primitive_info,
                   std::vector<uint32_t>& order)
    {
        Bounds node_bounds, centroids;
        for (uint32_t i = first; i < first + count; ++i)
        {
            node_bounds.grow(primitive_info[order[i]].min_corner,
                             primitive_info[order[i]].max_corner);
            centroids.grow(primitive_info[order[i]].centroid);
        }

        BinaryNode& node = binary_nodes[node_index];
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
                split_children(binary_nodes, node_index, first, count, count / 2, primitive_info,
                               order);
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

        split_children(binary_nodes, node_index, first, count, left_count, primitive_info, order);
    }

    void split_children(std::vector<BinaryNode>& binary_nodes, uint32_t node_index, uint32_t first,
                        uint32_t count, uint32_t left_count,
                        const std::vector<BuildPrimitive>& primitive_info,
                        std::vector<uint32_t>& order)
    {
        // binary_nodes may reallocate below: use indices, don't hold a reference
        uint32_t left_index = static_cast<uint32_t>(binary_nodes.size());
        binary_nodes.push_back({});
        binary_nodes.push_back({});
        binary_nodes[node_index].first = left_index;
        binary_nodes[node_index].count = 0;

        subdivide(binary_nodes, left_index, first, left_count, primitive_info, order);
        subdivide(binary_nodes, left_index + 1, first + left_count, count - left_count,
                  primitive_info, order);
    }

    /*
     * Turn the binary subtree at binary_index into a 4-wide node and return its index. The
     * node takes the binary node's two children, then keeps replacing the interior child with
     * the largest surface area (the one rays are most likely to enter) by its own two children
     * until it has four. A binary leaf at the root becomes a node with a single child.
     */
    uint32_t collapse(const std::vector<BinaryNode>& binary_nodes, uint32_t binary_index)
    {
        const BinaryNode& binary = binary_nodes[binary_index];
        uint32_t children[4];
        int child_count = 0;

        if (binary.count > 0)
        {
            children[child_count++] = binary_index;
        }
        else
        {
            children[child_count++] = binary.first;
            children[child_count++] = binary.first + 1;
        }

        while (child_count < 4)
        {
            int largest        = -1;
            float largest_area = -1.0f;
            for (int i = 0; i < child_count; ++i)
            {
                const BinaryNode& candidate = binary_nodes[children[i]];
                if (candidate.count == 0 && candidate.area() > largest_area)
                {
                    largest      = i;
                    largest_area = candidate.area();
                }
            }
            if (largest < 0)
            {
                break; // every child is a leaf
            }

            uint32_t opened         = children[largest];
            children[largest]       = binary_nodes[opened].first;
            children[child_count++] = binary_nodes[opened].first + 1;
        }

        // nodes may reallocate in the recursion below: use the index, don't hold a reference
        uint32_t node_index = static_cast<uint32_t>(nodes.size());
        nodes.push_back({});

        for (int lane = 0; lane < 4; ++lane)
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                // An empty lane gets an inverted box, which no ray can hit
                nodes[node_index].bounds[0][axis][lane] = INF;
                nodes[node_index].bounds[1][axis][lane] = -INF;
            }
            nodes[node_index].child[lane] = EMPTY_SLOT;
            nodes[node_index].count[lane] = 0;
        }

        for (int lane = 0; lane < child_count; ++lane)
        {
            const BinaryNode& child = binary_nodes[children[lane]];
            for (int axis = 0; axis < 3; ++axis)
            {
                nodes[node_index].bounds[0][axis][lane] = child.min_corner[axis];
                nodes[node_index].bounds[1][axis][lane] = child.max_corner[axis];
            }

            if (child.count > 0)
            {
                nodes[node_index].child[lane] = child.first;
                nodes[node_index].count[lane] = child.count;
            }
            else
            {
                uint32_t child_index          = collapse(binary_nodes, children[lane]);
                nodes[node_index].child[lane] = child_index;
            }
        }

        return node_index;
    }
};

} // namespace rt
