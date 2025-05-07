// Project Nucledian Source File
#pragma once

#include <transform.h>
#include <math/vector.h>
#include <engine/graphics/resources/model.h>

namespace nc
{
  
struct Appearance
{
  // Color modulation of the model.
  color4    color;
  Model     model;
  // Model transform (applied after the transform of the entity).
  Transform transform;
};

}

