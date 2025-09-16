#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdint.h>

// Common headers
#include "def.hpp"
#include "hittable.hpp"
#include "hittable_list.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "bvh_node.hpp"
#include "texture.hpp"
#include "quad.hpp"
#include "constant_medium.hpp"
#include "benchmark.hpp"
#include "triangle.hpp"

// Math
#include "rtm/ray.hpp"
#include "rtm/vector.hpp"
#include "rtm/constants.hpp"
#include "rtm/interval.hpp"
