#pragma once

#include <temp_math.h>

#include <glad/glad.h>

namespace nc
{

  /**
   * Compile - time type - safe uniform identificator.Global constants for uniform are predefined in
   * shaders::<shader_name>::<uniform_name>..
   */
  template<GLint location, typename T>
  struct Uniform
  {
    using UniformType = T;

    inline static constexpr GLint LOCATION = location;
  };

  namespace shaders
  {
    // Shader for drawing gizmos.
    namespace gizmo
    {
      #include <engine/graphics/shaders/gizmo.frag>
      #include <engine/graphics/shaders/gizmo.vert>

      inline constexpr Uniform<0, mat4>  TRANSFORM;
      inline constexpr Uniform<1, mat4>  VIEW;
      inline constexpr Uniform<2, mat4>  PROJECTION;
      inline constexpr Uniform<3, color> COLOR;
    }
  }

}
