#pragma once

#include <types.h>
#include <engine/core/resources.h>

#include <glad/glad.h>

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
  const u32 m_vertex_count = 0;

  GLuint m_vao = 0;
  GLuint m_vbo = 0;

  void unload();
};
using MeshHandle = ResourceHandle<Mesh>;

class MeshManager : public ResourceManager<Mesh>
{
public:
  // TODO: cancel singleton pattern, move instance probably to GraphicsSystem
  static MeshManager* instance();

  // TODO: ResourceHandle<Mesh> load(std::string_view path, ResourceLifetime lifetime);
  MeshHandle create(const f32* vertex_data, u32 size, ResourceLifetime lifetime);
  MeshHandle get_cube() const;
  // TODO: ResourceHandle<Mesh> get_sphere() const;
  // TODO: ResourceHandle<Mesh> get_capsule() const;
private:
  MeshManager();

  void create_cube();

  MeshHandle m_cube_mesh_handle = MeshHandle::invalid();
};

// Contains handles for precreated meshes of simple 3d shapes. Handles are valid as long as mesh manager lives.
class meshes
{
public:
  static MeshHandle cube();

private:
  meshes() {}
};

}
