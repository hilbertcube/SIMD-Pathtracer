#pragma once

#include "rt/math/interval.h"
#include "rt/math/vector.h"
#include "rt/math/ray.h"

namespace rt
{
class AABB
{
public:
    interval x, y, z;

    AABB() {} // The default AABB is empty, since intervals are empty by default.

    AABB(const interval& x, const interval& y, const interval& z) : x(x), y(y), z(z)
    {
        pad_to_minimums();
    }

    AABB(const point3f& corner_a, const point3f& corner_b)
    {
        // Treat the two points corner_a and corner_b as extrema for the bounding box, so we don't
        // require a particular minimum/maximum coordinate order.

        x = (corner_a[0] <= corner_b[0]) ? interval(corner_a[0], corner_b[0])
                                         : interval(corner_b[0], corner_a[0]);
        y = (corner_a[1] <= corner_b[1]) ? interval(corner_a[1], corner_b[1])
                                         : interval(corner_b[1], corner_a[1]);
        z = (corner_a[2] <= corner_b[2]) ? interval(corner_a[2], corner_b[2])
                                         : interval(corner_b[2], corner_a[2]);

        pad_to_minimums();
    }

    AABB(const AABB& first, const AABB& second)
    {
        x = interval(first.x, second.x);
        y = interval(first.y, second.y);
        z = interval(first.z, second.z);
    }

    const interval& axis_interval(int axis) const
    {
        if (axis == 1)
        {
            return y;
        }
        if (axis == 2)
        {
            return z;
        }
        return x;
    }

    bool hit(const ray& incoming, interval t_range) const
    {
        const point3f& origin  = incoming.origin();
        const vec3f& direction = incoming.direction();

        for (int axis = 0; axis < 3; axis++)
        {
            const interval& slab          = axis_interval(axis);
            const float inverse_direction = 1.0f / direction[axis];

            auto t_min_side = (slab.min - origin[axis]) * inverse_direction;
            auto t_max_side = (slab.max - origin[axis]) * inverse_direction;

            if (t_min_side < t_max_side)
            {
                if (t_min_side > t_range.min)
                {
                    t_range.min = t_min_side;
                }
                if (t_max_side < t_range.max)
                {
                    t_range.max = t_max_side;
                }
            }
            else
            {
                if (t_max_side > t_range.min)
                {
                    t_range.min = t_max_side;
                }
                if (t_min_side < t_range.max)
                {
                    t_range.max = t_min_side;
                }
            }

            // Inclusive, so flat (padded) boxes hit head-on aren't lost to float rounding
            if (t_range.max < t_range.min)
            {
                return false;
            }
        }
        return true;
    }

    int longest_axis() const
    {
        if (x.size() > y.size())
        {
            return x.size() > z.size() ? 0 : 2;
        }
        else
        {
            return y.size() > z.size() ? 1 : 2;
        }
    }

    static const AABB empty, universe;

private:
    // Pads any side narrower than delta
    void pad_to_minimums()
    {
        float delta = 0.0001f;
        if (x.size() < delta)
        {
            x = x.expand(delta);
        }
        if (y.size() < delta)
        {
            y = y.expand(delta);
        }
        if (z.size() < delta)
        {
            z = z.expand(delta);
        }
    }
};

const AABB AABB::empty    = AABB(interval::empty, interval::empty, interval::empty);
const AABB AABB::universe = AABB(interval::universe, interval::universe, interval::universe);

AABB operator+(const AABB& bounds, const vec3f& offset)
{
    return AABB(bounds.x + offset.x(), bounds.y + offset.y(), bounds.z + offset.z());
}

AABB operator+(const vec3f& offset, const AABB& bounds)
{
    return bounds + offset;
}

} // namespace rt
