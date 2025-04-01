#pragma once

#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/gl_types.h>
#include <types.h>

#include <vector>

namespace nc
{

/**
 * Mesh contains information about object geometry. This class represent a only a light-weight handler. Real mesh is
 * stored in GPU memory.
 * 
 * WARNING: Later on, there will be change to indect rendering, so this class contatne will probably change!
 */
class Mesh
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
  static Mesh invalid();

private:
  Mesh() {}

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
  * loading/creation in order to suport indirect rendering
  * TODO: Load mesh form file
*/
public:
  friend class Mesh;

  void init();

  /**
   * Creates mesh from vertex data and stores it in GPU memory.
   */
  Mesh create(ResLifetime lifetime, const f32* data, u32 count, GLenum draw_mode = GL_TRIANGLES);

  /**
   * Unloads all meshes with specified lifetime. 
   */
  void unload(ResLifetime lifetime);

  Mesh get_cube() const;


private:
  static inline u16 generation = 0;

  std::vector<Mesh>& get_storage(ResLifetime lifetime);

  std::vector<Mesh> m_level_meshes;
  std::vector<Mesh> m_game_meshes;
  Mesh              m_cube_mesh;
};

}
