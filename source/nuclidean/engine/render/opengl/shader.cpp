// Project Nuclidean Source File
#include <engine/render/opengl/shader.h>

#include <common.h>
#include <logging.h>
#include <metaprogramming.h>

#include <glad/glad.h>

#if NC_TESTS
#include <unit_test.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nc::render::opengl
{

#if NC_HOT_RELOAD
static constexpr std::string_view SOURCE_COMMENT = "// source: ";

//==============================================================================
static std::optional<std::string> parse_source_path(std::string_view glsl)
{
  if (!glsl.starts_with(SOURCE_COMMENT))
    return std::nullopt;

  glsl.remove_prefix(SOURCE_COMMENT.size());
  return std::string(glsl.substr(0, glsl.find('\n')));
}

//==============================================================================
static std::optional<std::string> compile_slang(const std::string& source_path)
{
  const std::filesystem::path name = std::filesystem::path(source_path).stem();
  const std::filesystem::path output = std::filesystem::temp_directory_path() / name;
  const std::string spirv = output.string() + ".spv";
  const std::string glsl = output.string() + ".glsl";

  const std::string slangc = std::format(
    "tools\\slangc.exe \"{}\" -target spirv -entry main -O0 -o \"{}\"",
    source_path, spirv
  );
  const std::string spirv_cross = std::format(
    "tools\\spirv-cross.exe \"{}\" --no-es --version 430 --output \"{}\"",
    spirv, glsl
  );

  std::optional<std::string> source;
  if (std::system(slangc.c_str()) == 0 && std::system(spirv_cross.c_str()) == 0)
  {
    std::ifstream file(glsl);
    source.emplace(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  }

  std::filesystem::remove(spirv);
  std::filesystem::remove(glsl);
  return source;
}

//==============================================================================
static std::filesystem::file_time_type get_newest_write_time(const std::vector<std::pair<std::string, GLenum>>& sources)
{
  // includes are not tracked, so every file next to the sources counts
  std::filesystem::file_time_type newest;
  for (const std::pair<std::string, GLenum>& source : sources)
  {
    std::error_code error;
    const std::filesystem::path directory = std::filesystem::path(source.first).parent_path();
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, error))
      newest = std::max(newest, entry.last_write_time(error));
  }
  return newest;
}
#endif

//==============================================================================
Shader::Shader(cstr compute_source)
  : Shader({{compute_source, GL_COMPUTE_SHADER}}) {}

//==============================================================================
Shader::Shader(cstr vertex_source, cstr fragment_source)
  : Shader({{vertex_source, GL_VERTEX_SHADER}, {fragment_source, GL_FRAGMENT_SHADER}}) {}

//==============================================================================
Shader::Shader(std::initializer_list<std::pair<cstr, GLenum>> shader_sources)
{
  std::vector<GLuint> shaders;
  shaders.reserve(shader_sources.size());

  auto delete_shaders = [&shaders]()
  {
    for (auto&& shader : shaders)
    {
      glDeleteShader(shader);
    }
  };

  // compile shader sources
  bool compile_error = false;
  for (auto&& [source, type] : shader_sources)
  {
#if NC_HOT_RELOAD
    if (const std::optional<std::string> source_path = parse_source_path(source))
      m_sources.emplace_back(*source_path, type);
#endif

    const std::optional<GLuint> shader = compile_shader(source, type);

    if (!shader)
    {
      compile_error = true;
      continue;
    }

    shaders.push_back(*shader);
  }

#if NC_HOT_RELOAD
  m_sources_write_time = get_newest_write_time(m_sources);
#endif

  // clean up on compile error
  if (compile_error)
  {
    delete_shaders();
    return;
  }

  // create and link shader program
  const std::optional<GLuint> shader_program = link_program(shaders);
  if (!shader_program)
  {
    delete_shaders();
    return;
  }

  delete_shaders();
  m_handle = *shader_program;
}

//==============================================================================
Shader::~Shader()
{
  if (m_handle)
    glDeleteProgram(m_handle);
}

//==============================================================================
Shader::Shader(Shader&& other) noexcept
  : m_handle(std::exchange(other.m_handle, 0))
#if NC_HOT_RELOAD
  , m_sources(std::move(other.m_sources))
  , m_sources_write_time(other.m_sources_write_time)
#endif
{}

//==============================================================================
Shader& Shader::operator=(Shader&& other) noexcept
{
  std::swap(m_handle, other.m_handle);
#if NC_HOT_RELOAD
  std::swap(m_sources, other.m_sources);
  std::swap(m_sources_write_time, other.m_sources_write_time);
#endif
  return *this;
}

#if NC_HOT_RELOAD
//==============================================================================
void Shader::reload()
{
  const std::filesystem::file_time_type write_time = get_newest_write_time(m_sources);
  if (write_time <= m_sources_write_time)
    return;

  // Updated even on failure, so a broken source is not recompiled every frame.
  m_sources_write_time = write_time;

  std::vector<std::string> sources;
  for (const auto& [source_path, type] : m_sources)
  {
    std::optional<std::string> source = compile_slang(source_path);
    if (!source)
    {
      nc_warn("[render][shader] failed to reload shader '{}', keeping the old one", source_path);
      return;
    }
    sources.push_back(std::move(*source));
  }

  Shader shader = sources.size() == 1
    ? Shader(sources[0].c_str())
    : Shader(sources[0].c_str(), sources[1].c_str());

  if (!shader.m_handle)
  {
    nc_warn("[render][shader] failed to reload shader '{}', keeping the old one", m_sources[0].first);
    return;
  }

  std::swap(m_handle, shader.m_handle);
  nc_log("[render][shader] reloaded shader '{}'", m_sources[0].first);
}
#endif

//==============================================================================
void Shader::use() const
{
  glUseProgram(m_handle);
}

//==============================================================================
void Shader::dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const
{
  use();
  glDispatchCompute(cast<GLuint>(group_x), cast<GLuint>(group_y), cast<GLuint>(group_z));
  if (sync)
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

//==============================================================================
std::optional<GLuint> Shader::compile_shader(cstr source, GLenum type) const
{
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
    glDeleteShader(shader);

    const std::string message = "[render][shader] compile error: " + std::string(info_log);
    nc_crit("{}", message);
    return std::nullopt;
  }

  return shader;
}

//==============================================================================
std::optional<GLuint> Shader::link_program(std::span<const GLuint> shaders) const
{
  const GLuint shader_program = glCreateProgram();
  for (GLuint shader : shaders)
  {
    glAttachShader(shader_program, shader);
  }
  glLinkProgram(shader_program);

  GLint success = 0;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetProgramInfoLog(shader_program, sizeof(info_log), nullptr, info_log);
    glDeleteProgram(shader_program);

    const std::string message = "[render][shader] link error: " + std::string(info_log);
    nc_crit("{}", message);
    return std::nullopt;
  }

  return shader_program;
}

#if NC_TESTS && NC_HOT_RELOAD
//==============================================================================
static bool shader_source_path_test(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(parse_source_path("// source: a/b.slang\n#version 430\n") == "a/b.slang");
  NC_TEST_ASSERT(parse_source_path("// source: a/b.slang") == "a/b.slang");
  NC_TEST_ASSERT(!parse_source_path("#version 430\n"));
  NC_TEST_ASSERT(!parse_source_path(""));
  // The comment is only recognized on the first line.
  NC_TEST_ASSERT(!parse_source_path("#version 430\n// source: a/b.slang\n"));
  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(shader_source_path_test)->name("Render Shader Source Path");
#endif

}
