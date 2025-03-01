#pragma once

#include <types.h>
#include <engine/core/resources.h>

#include <glad/glad.h>

#include <string_view>

namespace nc
{

/**
  * Contains data which are necessary for rendering a mesh.
  *
  * A mesh is typically created and managed by the `ResourceManager<Mesh>`. Direct interaction with the `Mesh` class
  * is ussually unnecessary; instead, it is recommended to use `ResourceHandle<Mesh>`.
  *
  * Currently, this class stores only VAO (Vertex Array Object) and VBO (Vertex Buffer Object). In the future, this
  * class content will probably change to support indirect rendering. At that point, it will probably store an offset
  * into a global mesh buffer or one of several mesh buffers.
  */
class Mesh
{
public:
  friend class ResourceManager<Mesh>;

  Mesh(GLuint vao, GLuint vbo, u32 vertex_count);

  GLuint get_vao()          const;
  GLuint get_vbo()          const;
  u32    get_vertex_count() const;

private:
  GLuint m_vao          = 0;
  GLuint m_vbo          = 0;
  const u32    m_vertex_count = 0;

  void unload();
};

class MeshManager : public ResourceManager<Mesh>
{
public:
  // TODO: cancel singleton pattern, move instance probably to GraphicsSystem
  static MeshManager* instance();

  // TODO: ResourceHandle<Mesh> load(std::string_view path, ResourceLifetime lifetime);
  ResourceHandle<Mesh> create(const f32* vertex_data, u32 size, ResourceLifetime lifetime);
  ResourceHandle<Mesh> get_cube() const;
  // TODO: ResourceHandle<Mesh> get_sphere() const;
  // TODO: ResourceHandle<Mesh> get_capsule() const;
private:
  MeshManager();

  void create_cube();

  ResourceHandle<Mesh> m_cube_mesh_handle = ResourceHandle<Mesh>::invalid();
};

/**
  * Class for manupulation with a shader.
  *
  * A material is typically created and managed by the `ResourceManager<Material>`. Direct interaction with the
  * `Material` class is ussually unnecessary; instead, it is recommended to use `ResourceHandle<Material>`.
  */
class Material
{
public:
  Material(GLuint shader_program);

  void use() const;

  // TODO: set uniform

private:
  const GLuint m_shader_program;
};

// TODO: textures

}
