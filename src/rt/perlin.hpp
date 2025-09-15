#pragma once
#include "rtm/vector.hpp"
#include "rtm/random.hpp"

namespace rt {
class perlin
{
public:
    perlin() {
        for (int i = 0; i < point_count; i++) {
            rand_float[i] = unit_vector(vec3f::random(-1, 1));
        }

        perlin_generate_perm(perm_x);
        perlin_generate_perm(perm_y);
        perlin_generate_perm(perm_z);
    }
    ~perlin() {}

    // linearly interpolate to smooth out texture
    float noise_smooth(const point3f& p) const {
        auto u = p.x() - std::floor(p.x());
        auto v = p.y() - std::floor(p.y());
        auto w = p.z() - std::floor(p.z());

        auto i = int(std::floor(p.x()));
        auto j = int(std::floor(p.y()));
        auto k = int(std::floor(p.z()));
        vec3f c[2][2][2];

        for (int di=0; di < 2; di++)
            for (int dj=0; dj < 2; dj++)
                for (int dk=0; dk < 2; dk++)
                    c[di][dj][dk] = 
                    rand_float[perm_x[(i+di) & 255] ^ perm_y[(j+dj) & 255] ^ perm_z[(k+dk) & 255]];

        return perlin_interp(c, u, v, w);
    }

    // Very often, a composite noise that has multiple summed frequencies is used. 
    // This is usually called turbulence, and is a sum of repeated calls to noise
    float turbulence(const point3f& p, int depth) const {
        float accum    = 0.0f;
        float weight   = 1.0f;
        point3f temp_p = p;
        
        for (int i = 0; i < depth; i++) {
            accum += weight * noise_smooth(temp_p);
            weight *= 0.5f;
            temp_p *= 2;
        }

        return std::fabs(accum);
    }
private:
    static const int point_count = 256;
    vec3f rand_float[point_count];
    int perm_x[point_count];
    int perm_y[point_count];
    int perm_z[point_count];

    static void perlin_generate_perm(int* p) {
        for (int i = 0; i < point_count; i++)
            p[i] = i;

        permute(p, point_count);
    }

    static void permute(int* p, int n) {
        for (int i = n-1; i > 0; i--) {
            int target = random_int(0, i);
            int tmp = p[i];
            p[i] = p[target];
            p[target] = tmp;
        }
    }

    static FORCE_INLINE float fade(float t) {
        return t * t * (3 - 2 * t);
    }

    static float perlin_interp(const vec3f c[2][2][2], float u, float v, float w) {
        auto uu = fade(u);
        auto vv = fade(v);
        auto ww = fade(w);
        auto accum = 0.0;

        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++) {
                    vec3f weight_v(u-i, v-j, w-k);
                    accum += (i*uu + (1-i)*(1-uu))
                           * (j*vv + (1-j)*(1-vv))
                           * (k*ww + (1-k)*(1-ww))
                           * dot(c[i][j][k], weight_v);
                }

        return accum;
    }
};

}