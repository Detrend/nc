// Project Nucledian Source File
#pragma once

#include <engine/graphics/gl_types.h>

namespace nc
{
  /**
   * Compile-time type-safe uniform identificator. Global constants for uniform are predefined in
   * shaders::<shader_name>::<uniform_name>..
   */
  template<GLint location, typename T>
  struct Uniform
  {
    using UniformType = T;

    inline static constexpr GLint LOCATION = location;
  };
}
