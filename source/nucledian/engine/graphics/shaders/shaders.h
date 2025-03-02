#pragma once

#include <temp_math.h>

#include <glad/glad.h>

#include <string_view>

namespace nc
{

  /**
   * Compile - time type - safe uniform identificator.Global constants for uniform are predefined in
   * shaders::<shader_name>::<uniform_name>..
   */
  template<const std::string_view& shader_name, GLint location, typename T>
  struct Uniform
  {
    using UniformType = T;

    inline static constexpr std::string_view SHADER_NAME = shader_name;
    inline static constexpr GLint LOCATION = location;
  };

  namespace shaders
  {
    // Shader for drawing gizmos.
    namespace gizmo
    {
      #include "gizmo.vert"
      #include "gizmo.frag"

      inline constexpr std::string_view NAME = "gizmo";

      inline constexpr Uniform<NAME, 0, mat4> TRANSFORM;
      inline constexpr Uniform<NAME, 1, mat4> VIEW;
      inline constexpr Uniform<NAME, 2, mat4> PROJECTION;
      inline constexpr Uniform<NAME, 3, vec3> COLOR;
    }
  }

}
