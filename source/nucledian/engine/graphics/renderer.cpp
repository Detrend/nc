#include "renderer.h"

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

#include <iterator>

namespace nc
{

//==============================================================================
// TODO: proper shaders

constexpr const char* vertex_shader_source = // vertex shader:
R"(
  #version 430 core
  layout (location = 0) in vec3 in_pos;
  layout (location = 1) in vec3 in_normal;

  out vec3 normal;

  uniform mat4 view;
  uniform mat4 projection;
  uniform mat4 transform;

  void main()
  {
    gl_Position = projection * view * transform * vec4(in_pos, 1.0f);
    normal = mat3(transpose(inverse(transform))) * in_normal;
  }
)";

//==============================================================================
// TODO: proper shaders

constexpr const char* fragment_shader_source = // fragment shader:
R"(
  #version 430 core
  in vec3 normal;

  out vec4 out_color;

  uniform vec3 color;

  void main()
  {
    vec3 light_color = vec3(1.0f, 1.0f, 1.0f);
    vec3 light_direction = normalize(-vec3(-0.2f, -0.8f, -0.2f));

    vec3 ambient = 0.5f * light_color;
    vec3 diffuse = max(dot(normal, light_direction), 0.0f) * light_color;

    out_color = vec4((ambient + diffuse) * color, 1.0f);
  }
)";

//==============================================================================
Model::Model(MeshHandle mesh)
  : mesh(mesh) {}

//==============================================================================
Gizmo::Gizmo(MeshHandle mesh, const mat4& transform, const color& color, f32 ttl)
  : m_mesh_handle(mesh), m_transform(transform), m_color(color), m_ttl(ttl) {}

//==============================================================================
MeshHandle Gizmo::get_mesh() const
{
  return m_mesh_handle;
}

//==============================================================================
void Gizmo::set_mesh(MeshHandle mesh_handle)
{
  m_mesh_handle = mesh_handle;
}

//==============================================================================
mat4 Gizmo::get_transform() const
{
  return m_transform;
}

//==============================================================================
void Gizmo::set_transform(const mat4& transform)
{
  m_transform = transform;
}

//==============================================================================
color Gizmo::get_color() const
{
  return m_color;
}

//==============================================================================
void Gizmo::set_color(const color& color)
{
  m_color = color;
}

//==============================================================================
Renderer::~Renderer()
{
  glDeleteProgram(m_gizmos_shader_program);
  m_gizmos_shader_program = 0;
}

//==============================================================================
void Renderer::init()
{
  // compile gizmos shader
  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glCompileShader(vertex_shader);

  const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(fragment_shader);

  m_gizmos_shader_program = glCreateProgram();
  glAttachShader(m_gizmos_shader_program, vertex_shader);
  glAttachShader(m_gizmos_shader_program, fragment_shader);
  glLinkProgram(m_gizmos_shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // setup projection matrix
  const mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  glUseProgram(m_gizmos_shader_program);
  glUniformMatrix4fv(glGetUniformLocation(m_gizmos_shader_program, "projection"), 1, GL_FALSE, value_ptr(projection));
  glUseProgram(0);

  // TODO: temporary cube gizmo
  m_temp_cube_gizmo1 = create_gizmo(Meshes::cube(),  vec3::X, colors::RED , 5.0f);
  m_temp_cube_gizmo2 = create_gizmo(Meshes::cube(), -vec3::X, colors::BLUE, 9.0f);
}

//==============================================================================
void Renderer::render() const
{
  // TODO: indirect rendering

  // TODO: render entities
  // TODO: render secors

  render_gizmos();
}

//==============================================================================
void Renderer::update_gizmos(f32 delta_seconds)
{
  std::vector<u64> to_delete;

  for (auto& [id, gizmo] : m_gizmos)
  {
    gizmo.m_ttl -= delta_seconds;
    if (gizmo.m_ttl < 0.0f)
    {
      to_delete.push_back(id);
    }
  }

  for (auto& id : to_delete)
  {
    m_gizmos.erase(id);
  }
}

//==============================================================================
GizmoPtr Renderer::create_gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color, f32 ttl)
{
  u64 id = m_next_gizmo_id++;
  auto [it, _] = m_gizmos.emplace(id, Gizmo(mesh_handle, transform, color, ttl));

  auto deleter = [this, id](const Gizmo*)
  {
    auto it = m_gizmos.find(id);

    if (it != m_gizmos.end())
    {
      m_gizmos.erase(it);
    }
  };

  return GizmoPtr(&it->second, deleter);
}

//==============================================================================
GizmoPtr Renderer::create_gizmo(MeshHandle mesh_handle, const vec3& position, const color& color, f32 ttl)
{
  return create_gizmo(mesh_handle, translate(mat4(1.0f), position), color, ttl);
}

//==============================================================================
void Renderer::render_gizmos() const
{
  // TODO: render only visible gizmos

  const MeshManager* meshes = MeshManager::instance();
  glUseProgram(m_gizmos_shader_program);

  const mat4 view = get_engine().get_module<GraphicsSystem>().get_debug_camera().get_view();
  glad_glUniformMatrix4fv(glGetUniformLocation(m_gizmos_shader_program, "view"), 1, GL_FALSE, value_ptr(view));

  const GLint transform_location = glGetUniformLocation(m_gizmos_shader_program, "transform");
  const GLint color_location = glGetUniformLocation(m_gizmos_shader_program, "color");

  for (const auto& [_, gizmo] : m_gizmos)
  {
    const Mesh& mesh = meshes->get_resource(gizmo.m_mesh_handle);
    const color color = gizmo.get_color();

    glUniformMatrix4fv(transform_location, 1, GL_FALSE, value_ptr(gizmo.get_transform()));
    glUniform3f(color_location, color.r, color.g, color.b);
    glBindVertexArray(mesh.get_vao());
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

}