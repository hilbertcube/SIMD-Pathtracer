#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdint.h>

// Umbrella header: everything a scene needs. Includes are written from src/.

// Utilities
#include "rt/util/def.h"
#include "rt/util/timer.h"

// Math
#include "rt/math/constants.h"
#include "rt/math/interval.h"
#include "rt/math/ray.h"
#include "rt/math/vector.h"

// Geometry and acceleration
#include "rt/geometry/hittable.h"
#include "rt/geometry/hittable_list.h"
#include "rt/geometry/sphere.h"
#include "rt/geometry/quad.h"
#include "rt/geometry/triangle.h"
#include "rt/geometry/constant_medium.h"
#include "rt/acceleration/bvh_node.h"

// Materials
#include "rt/materials/materials.h"
#include "rt/materials/common/pdf.h"
#include "rt/materials/textures/texture.h"

// Rendering
#include "rt/render/camera.h"
