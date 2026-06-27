// Project Nuclidean Source File
#pragma once

#include <config.h>

#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/gl_types.h>
#include <types.h>
#include <math/vector.h>

#include <vector>
#include <memory>
#include <span>   // std::span

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
  u16         m_index        = 0;
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

  static MeshManager& get();

  void init();
  /**
   * Creates mesh from vertex data and stores it in GPU memory.
   */
  MeshHandle create(ResLifetime lifetime, const f32* data, u32 count, GLenum draw_mode = GL_TRIANGLES);
  /**
   * Creates mesh, on which texture can be applied, from vertex data and stores it in GPU memory.
   */
  MeshHandle create_texturable(ResLifetime lifetime, const f32* data, u32 count, GLenum draw_mode = GL_TRIANGLES);
  /**
   * Creates mesh, for a sector, from vertex data and stores it in GPU memory.
   */
  MeshHandle create_sector(ResLifetime lifetime, const f32* data, u32 count, GLenum draw_mode = GL_TRIANGLES);
#if NC_EDITOR
  /**
  * Creates a mesh for an editor. Can be used with GL_LINES, GL_TRIANGLES, GL_LINE_STRIP or GL_LINE_LOOP.
  * The mesh does not get registered into the list of resources and therefore has to be deallocated by
  * the author himself after it is no longer needed.
  */
  MeshHandle create_editor_primitive(std::span<vec2> data, GLenum draw_mode = GL_LINES);
  /**
  * Deallocates previously created editor mesh, freeing the resources. Has to be called before termination
  * of graphics system and OpenGL.
  */
  void destroy_editor_primitive(MeshHandle& handle);
#endif
  /*
  * Unloads the old mesh and recreates it once again.
  */
  void recreate_sector(MeshHandle& handle, const f32* data, u32 count);
  /**
   * Unloads all meshes with specified lifetime. 
   */
  void unload(ResLifetime lifetime);

  MeshHandle get_cube() const;
  MeshHandle get_line() const;
  MeshHandle get_quad() const;
  MeshHandle get_texturable_quad() const;
  MeshHandle get_screen_quad() const;

private:
  inline static std::unique_ptr<MeshManager> m_instance = nullptr;
  MeshManager() {}

  void populate_sector_mesh(MeshHandle& mesh, const f32* data, u32 count);

  static inline u16 m_generation = 0;

  std::vector<MeshHandle>& get_storage(ResLifetime lifetime);

  std::vector<MeshHandle> m_level_meshes;
  std::vector<MeshHandle> m_game_meshes;
  MeshHandle              m_cube_mesh;
  MeshHandle              m_line_mesh;
  MeshHandle              m_quad_mesh;
  MeshHandle              m_texturable_quad_mesh;
  MeshHandle              m_screen_quad_mesh;
};

}
