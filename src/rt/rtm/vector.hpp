#pragma once

//#define _USE_SIMD_
#ifndef _USE_SIMD_
    #include "vecN.hpp"
#else
    #include "vec3_SIMD.hpp"
#endif
#include "vec_functions.hpp"

namespace rt {
// === CONVENIENT ALIAS ===
// Common vector type aliases
template<typename T> using Vec2 = VecN<T, 2>;
template<typename T> using Vec3 = VecN<T, 3>;
template<typename T> using Vec4 = VecN<T, 4>;

// Specific vector type aliases
using vec2ui = Vec2<unsigned int>;
using vec2i  = Vec2<int>;
using vec2f  = Vec2<float>;
using vec2d  = Vec2<double>;

using vec3ui = Vec3<unsigned int>;
using vec3i  = Vec3<int>;
using vec3f  = Vec3<float>;
using vec3d  = Vec3<double>;

using vec4ui = Vec4<unsigned int>;
using vec4i  = Vec4<int>;
using vec4f  = Vec4<float>;
using vec4d  = Vec4<double>;

using point2i = Vec2<int>;
using point2f = Vec2<float>;
using point2d = Vec2<double>;

using point3i = Vec3<int>;
using point3f = Vec3<float>;
using point3d = Vec3<double>;

using point4i = Vec4<int>;
using point4f = Vec4<float>;
using point4d = Vec4<double>;

using color = vec3f;
}
