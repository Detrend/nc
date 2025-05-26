// Project Nucledian Source File
#pragma once

#include <transform.h>
#include <math/vector.h>
#include <engine/graphics/resources/model.h>
#include <engine/graphics/resources/texture.h>

namespace nc
{

struct Appearance
{
  TextureHandle texture = TextureHandle::invalid();
  f32 scale = 1.0f;
};

}

