#pragma once

#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>

namespace nc
{

// Collection of data needed to render an object.
struct Model
{
  Mesh mesh = Mesh::invalid();
  Material material = Material::invalid();
};

}