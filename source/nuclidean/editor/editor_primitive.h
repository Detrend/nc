// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <engine/graphics/resources/mesh.h> // MeshHandle
#include <engine/graphics/gl_types.h>       // GL_LINES
#include <editor/editor_types.h>            // EditorID
#include <math/matrix.h>                    // mat3
#include <math/lingebra.h>                  // identity<>

#include <aabb.h> // aabb2

#include <memory> // std::shared_ptr, std::enable_shared_from_this
#include <span>   // std::span
#include <vector> // std::vector
#include <optional>

namespace nc
{

struct EditorPrimitiveRenderingProperties
{
  color4   color      = colors::WHITE;
  f32      line_width = 1.0f;
  bool     visible    = true;
  mat3     transform  = identity<mat3>();
  s64      order     = 0;
  EditorID author_id = INVALID_EDITOR_ID; // Optional author ID

  bool override_color      : 1 = false;
  bool override_line_width : 1 = false;
  bool override_visiblity  : 1 = false;
  bool override_transform  : 1 = false;
  bool override_order      : 1 = false;
  bool override_author     : 1 = false;

  void merge_with(const EditorPrimitiveRenderingProperties& other)
  {
    if (other.override_color)
      this->color = other.color;

    if (other.override_visiblity)
      this->visible = other.visible;

    if (other.override_transform)
      this->transform = other.transform;

    if (other.override_line_width)
      this->line_width = other.line_width;

    if (other.override_order)
      this->order = other.order;

    if (other.override_author)
      this->author_id = other.author_id;
  }
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

  // We store these separately so they can be modified
  EditorPrimitiveRenderingProperties properties = EditorPrimitiveRenderingProperties{};
};

using EditorPrimitivePtr = std::shared_ptr<EditorPrimitive>;

struct RenderList
{
  struct PrimitiveAndProperties
  {
    EditorPrimitivePtr                 primitive;
    EditorPrimitiveRenderingProperties unique_props;
  };

  void push_back(EditorPrimitivePtr primitive)
  {
    primitives.emplace_back(primitive, EditorPrimitiveRenderingProperties{});
  }

  void push_back(EditorPrimitivePtr primitive, const EditorPrimitiveRenderingProperties& props)
  {
    primitives.emplace_back(primitive, props);
  }

  std::vector<PrimitiveAndProperties> primitives;
};

}

#endif // #if NC_EDITOR
