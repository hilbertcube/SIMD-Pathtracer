#pragma once

// Every material, one per header. Include a single one when that is all a file needs.

#include "rt/materials/material.h"

#include "rt/materials/diffuse/lambertian.h"
#include "rt/materials/metals/metal.h"
#include "rt/materials/glass/dielectrics.h"
#include "rt/materials/lights/diffuse_light.h"
#include "rt/materials/media/isotropic.h"
#include "rt/materials/layered/coated.h"
#include "rt/materials/glass/tinted_dielectric.h"
#include "rt/materials/glass/dispersive_dielectric.h"
#include "rt/materials/glass/rough_dielectric.h"
#include "rt/materials/metals/conductor.h"
#include "rt/materials/metals/brushed_metal.h"
#include "rt/materials/diffuse/oren_nayar.h"
#include "rt/materials/layered/iridescent.h"
#include "rt/materials/diffuse/velvet.h"
#include "rt/materials/layered/bumped.h"
#include "rt/materials/media/mist.h"
#include "rt/materials/media/glowing_medium.h"
