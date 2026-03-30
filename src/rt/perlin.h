#pragma once
#include "rtm/vector.h"
#include "rtm/random.h"

namespace rt
{
class perlin
{
public:
    perlin()
    {
        for (int i = 0; i < point_count; i++)
        {
            random_gradients[i] = unit_vector(vec3f::random(-1, 1));
        }
        generate_permutation(permutation_x);
        generate_permutation(permutation_y);
        generate_permutation(permutation_z);
    }

    ~perlin() {}

    // linearly interpolate to smooth out texture
    float noise_smooth(const point3f& point) const
    {
        auto fraction_x = point.x() - std::floor(point.x());
        auto fraction_y = point.y() - std::floor(point.y());
        auto fraction_z = point.z() - std::floor(point.z());

        auto cell_x = int(std::floor(point.x()));
        auto cell_y = int(std::floor(point.y()));
        auto cell_z = int(std::floor(point.z()));
        vec3f corners[2][2][2];

        for (int offset_x = 0; offset_x < 2; offset_x++)
        {
            for (int offset_y = 0; offset_y < 2; offset_y++)
            {
                for (int offset_z = 0; offset_z < 2; offset_z++)
                {
                    corners[offset_x][offset_y][offset_z] =
                        random_gradients[permutation_x[(cell_x + offset_x) & 255] ^
                                         permutation_y[(cell_y + offset_y) & 255] ^
                                         permutation_z[(cell_z + offset_z) & 255]];
                }
            }
        }

        return interpolate(corners, fraction_x, fraction_y, fraction_z);
    }

    // Very often, a composite noise that has multiple summed frequencies is used.
    // This is usually called turbulence, and is a sum of repeated calls to noise
    float turbulence(const point3f& point, int depth) const
    {
        float sum            = 0.0f;
        float weight         = 1.0f;
        point3f sample_point = point;

        for (int i = 0; i < depth; i++)
        {
            sum += weight * noise_smooth(sample_point);
            weight *= 0.5f;
            sample_point *= 2;
        }

        return std::fabs(sum);
    }

private:
    static const int point_count = 256;
    vec3f random_gradients[point_count];
    int permutation_x[point_count];
    int permutation_y[point_count];
    int permutation_z[point_count];

    static void generate_permutation(int* permutation)
    {
        for (int i = 0; i < point_count; i++)
        {
            permutation[i] = i;
        }

        permute(permutation, point_count);
    }

    static void permute(int* permutation, int count)
    {
        for (int i = count - 1; i > 0; i--)
        {
            int target          = random_int(0, i);
            int temporary       = permutation[i];
            permutation[i]      = permutation[target];
            permutation[target] = temporary;
        }
    }

    static FORCE_INLINE float fade(float t) { return t * t * (3 - 2 * t); }

    static float interpolate(const vec3f corners[2][2][2], float fraction_x, float fraction_y,
                             float fraction_z)
    {
        auto smooth_x = fade(fraction_x);
        auto smooth_y = fade(fraction_y);
        auto smooth_z = fade(fraction_z);
        float sum     = 0.0f;

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    vec3f offset(fraction_x - i, fraction_y - j, fraction_z - k);
                    sum += (i * smooth_x + (1 - i) * (1 - smooth_x)) *
                           (j * smooth_y + (1 - j) * (1 - smooth_y)) *
                           (k * smooth_z + (1 - k) * (1 - smooth_z)) *
                           dot(corners[i][j][k], offset);
                }
            }
        }

        return sum;
    }
};

} // namespace rt
