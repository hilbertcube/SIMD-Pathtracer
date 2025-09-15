#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdint.h>

// Common headers
#include "def.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include "bvh_node.h"
#include "texture.h"
#include "quad.h"
#include "constant_medium.h"
#include "benchmark.h"

// Math
#include "rtm/ray.h"
#include "rtm/vector.h"
#include "rtm/constants.h"
#include "rtm/interval.h"
