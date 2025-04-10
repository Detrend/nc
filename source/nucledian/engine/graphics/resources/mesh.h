#pragma once

#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/gl_types.h>
#include <types.h>

#include <vector>
#include <memory>

namespace nc
{

/**
 * Mesh contains information about object geometry. This class represent a only a light-weight handler. Real mesh is
 * stored in GPU memory.
 * 
 * WARNING: Later on, there will be change to indirect rendering, so this class content will probably change!
 */
class MeshHandle
{
public:
  friend class MeshManager;

  GLuint get_vao() const;
  GLuint get_vbo() const;
  u32 get_vertex_count() const;
  GLenum get_draw_mode() const;

  bool is_valid() const;
  operator bool() const;

  // Gets a invalid mesh.
  static MeshHandle invalid();

private:
  MeshHandle() {}

  ResLifetime m_lifetime     = ResLifetime::None;
  u16         m_generation   = 0;
  GLuint      m_vao          = 0;
  GLuint      m_vbo          = 0;
  u32         m_vertex_count = 0;
  GLenum      m_draw_mode    = GL_TRIANGLES;
};

class MeshManager
{
/*
  * TODO: In future, there will be need for function for which would indicate start and end of batch mesh
  * loading/creation in order to support indirect rendering
  * TODO: Load mesh form file
*/
public:
  friend class MeshHandle;

  static MeshManager& instance();

  void init();
  /**
   * Creates mesh from vertex data and stores it in GPU memory.
   */
  MeshHandle create(ResLifetime lifetime, const f32* data, u32 count, GLenum draw_mode = GL_TRIANGLES);
  /**
   * Unloads all meshes with specified lifetime. 
   */
  void unload(ResLifetime lifetime);
  MeshHandle get_cube() const;
  MeshHandle get_line() const;

private:
  inline static std::unique_ptr<MeshManager> m_instance = nullptr;
  MeshManager() {}

  static inline u16 m_generation = 0;

  std::vector<MeshHandle>& get_storage(ResLifetime lifetime);

  std::vector<MeshHandle> m_level_meshes;
  std::vector<MeshHandle> m_game_meshes;
  MeshHandle              m_cube_mesh;
  MeshHandle              m_line_mesh;
};

}
