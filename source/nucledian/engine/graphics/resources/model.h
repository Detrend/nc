#pragma once

#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/shader_program.h>

namespace nc
{

// Collection of handles needed to render an object.
struct Model
{
  MeshHandle mesh = MeshHandle::invalid();
  ShaderProgramHandle material = ShaderProgramHandle::invalid();
};

}