#pragma once

#include <types.h>
#include <temp_math.h>
#include <engine/graphics/resources/model.h>

namespace nc
{

// Component which enables entities to be rendered in the scene.
struct RenderComponent
{
  ModelHandle model_handle;
  f32 scale;
  f32 y_offset;
  color model_color;
};

}