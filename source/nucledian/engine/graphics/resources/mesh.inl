#pragma once

#include <types.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/res_lifetime.h>

namespace nc
{

//==============================================================================
template<ResLifetime lifetime>
inline Mesh MeshManager::create(const f32* data, u32 count, GLenum draw_mode)
{
  Mesh mesh;
  mesh.m_lifetime = lifetime;
  mesh.m_generation = MeshManager::generation;
  mesh.m_draw_mode = draw_mode;
  mesh.m_vertex_count = count / 6; // 3 floats per position + 3 floats per normal
  
  // generate buffers
  glGenVertexArrays(1, &mesh.m_vao);
  glGenBuffers(1, &mesh.m_vbo);

  // setup vao
  glBindVertexArray(mesh.m_vao);
  // vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vbo);
  glBufferData(GL_ARRAY_BUFFER, count * sizeof(f32), data, GL_STATIC_DRAW);
  //position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);
  // normal attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), reinterpret_cast<void*>(3 * sizeof(f32)));
  glEnableVertexAttribArray(1);

  // store created mesh
  auto& storage = this->get_storage<lifetime>();
  storage.push_back(mesh);

  return mesh;
}

//==============================================================================
template<ResLifetime lifetime>
inline void MeshManager::unload()
{
  auto& storage = this->get_storage<lifetime>();
  std::vector<GLuint> vao_to_delete;
  std::vector<GLuint> vbo_to_delete;

  for (auto& mesh : storage)
  {
    if (mesh.m_vao != 0)
    {
      vao_to_delete.push_back(mesh.m_vao);
      mesh.m_vao = 0;
    }

    if (mesh.m_vbo != 0)
    {
      vbo_to_delete.push_back(mesh.m_vbo);
      mesh.m_vbo = 0;
    }
  }

  glDeleteVertexArrays(static_cast<GLsizei>(vao_to_delete.size()), vao_to_delete.data());
  glDeleteBuffers(static_cast<GLsizei>(vbo_to_delete.size()), vbo_to_delete.data());
  storage.clear();

  if constexpr (lifetime == ResLifetime::Game) {
    MeshManager::generation++;
  }
}

//==============================================================================
template<ResLifetime lifetime>
inline std::vector<Mesh>& MeshManager::get_storage()
{
  static_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if constexpr (lifetime == ResLifetime::Level)
  {
    return m_level_meshes;
  }
  else
  {
    return m_game_meshes;
  }
}

}
