#pragma once

#include <types.h>

namespace nc
{

/**
 * Class for manupulation with a shader.
 * 
 * A material is typically created and managed by the `ResourceManager<Material>`. Direct interaction with the
 * `Material` class is ussually unnecessary; instead, it is recommended to use `ResourceHandle<Material>`.
 */
class Material
{
public:
  // TODO: Material(u32 shader_program);

  // TODO: set uniform
  // TODO: use shader program

private:
  const u32 m_shader_program;
};

}

