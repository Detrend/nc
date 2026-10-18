// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <types.h>
#include <editor/editor_primitive.h>

#include <math/vector.h>
#include <common.h>

#include <vector>
#include <variant>

namespace nc
{

struct EditorImpl;

struct EditorGenericRenderData
{
  // Empty by purpose
  void get_render_data(const EditorImpl& /*impl*/, RenderList& /*list*/)
  {

  }
};

struct EditorWall
{
  ivec2 pt;
};

struct EditorSectorRenderData
{
  using IndexList = std::vector<u16>;

  EditorPrimitivePtr      render_data_surface = std::make_shared<EditorPrimitive>();
  std::vector<EditorWall> walls;
  u64                     id;

  std::vector<std::vector<EditorWall>> holes;

  // The outer walls followed by the walls of all holes. This is what the convex parts index into,
  // not the walls themselves.
  std::vector<ivec2>     surface_points;
  std::vector<IndexList> convex_parts; // Used for selection

  void get_render_data(const EditorImpl& impl, RenderList& list);
  void convexify_surface();
  void recompute_render_data();
};

struct EditorLineRenderData
{
  EditorPrimitivePtr render_data_line = std::make_shared<EditorPrimitive>();
  u64                id = 0;

  void recreate_render_data(ivec2 start, ivec2 end)
  {
    vec2 points[2] = {cast<vec2>(start), cast<vec2>(end)};
    render_data_line->refresh_gpu_data(std::span<vec2>(points, points+2));
    render_data_line->properties.color = colors::WHITE;
    render_data_line->properties.order = 66;
    render_data_line->properties.author_id = id;
  }

  void get_render_data(const EditorImpl& /*impl*/, RenderList& list)
  {
    if (render_data_line->is_valid())
    {
      list.push_back(render_data_line);
    }
  }
};

struct EditorPointRenderData
{
  u64         id = 0;
  EditorCoord coords;

  void get_render_data(const EditorImpl& impl, RenderList& list);
};

using EditorObjectRenderData = std::variant<EditorGenericRenderData, EditorSectorRenderData, EditorLineRenderData, EditorPointRenderData>;

}

#endif // #if NC_EDITOR
