// Project Nuclidean Source File
#pragma once

#include <engine/graphics/resources/mesh.h> // MeshHandle
#include <engine/graphics/gl_types.h>       // GL_LINES

#include <aabb.h> // aabb2

#include <memory> // std::shared_ptr, std::enable_shared_from_this
#include <span>   // std::span
#include <vector> // std::vector

namespace nc
{

enum class EditorPrimitiveType : u8
{
  generic = 0, // pretty much nothing
  sector,
  entity,
  grid,
};

struct EditorPrimitiveRenderingProperties
{
  color4 color      = colors::WHITE;
  f32    line_width = 1.0f;
  bool   visible    = true;
};

struct EditorPrimitive : public std::enable_shared_from_this<EditorPrimitive>
{
  EditorPrimitive() = default;

  // Disable any moving or construction whatsoever
  EditorPrimitive(const EditorPrimitive&)            = delete;
  EditorPrimitive& operator=(const EditorPrimitive&) = delete;
  EditorPrimitive(EditorPrimitive&&)                 = delete;
  EditorPrimitive& operator=(EditorPrimitive&&)      = delete;

  bool is_valid() const;
  void refresh_gpu_data(std::span<vec2> points, GLenum primitive_type = GL_LINES);

  // Deallocates the mesh automatically
  ~EditorPrimitive();

  MeshHandle handle = MeshHandle::invalid();
  aabb2      bbox   = aabb2{};
  s64        order  = 0;

  // We store these separately so they can be modified
  EditorPrimitiveRenderingProperties properties = EditorPrimitiveRenderingProperties{};

  // Data used for identifying the property later
  EditorPrimitiveType type = EditorPrimitiveType::generic;
  struct
  {
    u8  type;
    u64 id;
  } sector;
  struct
  {
    u64 id;
  } entity;
};

using EditorPrimitivePtr = std::shared_ptr<EditorPrimitive>;
using RenderList         = std::vector<EditorPrimitivePtr>;

}
