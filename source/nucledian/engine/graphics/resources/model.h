#pragma once

#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>

namespace nc
{

// Collection of handles needed to render an object.
struct Model
{
  MeshHandle mesh = MeshHandle::invalid();
  MaterialHandle material = MaterialHandle::invalid();
};

}