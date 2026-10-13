// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
#include <engine/render/opengl/gl_types.h>

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace nc::render::opengl
{

class Shader
{
public:
  Shader() = default;
  explicit Shader(cstr compute_source);
  Shader(cstr vertex_source, cstr fragment_source);
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;

  void use() const;
  void dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const;

#if NC_HOT_RELOAD
  // Recompile the program if its Slang sources changed, keep the current one and warn on failure.
  void reload();
#endif

private:
  GLuint m_handle = 0;

#if NC_HOT_RELOAD
  // Slang source path and type of each stage, read from the source comment at the top of its GLSL.
  std::vector<std::pair<std::string, GLenum>> m_sources;
  // Newest write time of the files next to the Slang sources the program was last built from.
  std::filesystem::file_time_type m_sources_write_time;
#endif

  explicit Shader(std::initializer_list<std::pair<cstr, GLenum>> shader_sources);

  std::optional<GLuint> compile_shader(cstr source, GLenum type) const;
  std::optional<GLuint> link_program(std::span<const GLuint> shaders) const;

};

}
