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

struct EditorGenericRenderData
{
  // Empty by purpose
  void get_render_data(RenderList& /*list*/)
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

  EditorPrimitivePtr      render_data_lines   = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr      render_data_surface = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr      render_data_splits  = std::make_shared<EditorPrimitive>();
  std::vector<EditorWall> walls;
  u64                     id;

  std::vector<std::vector<EditorWall>> holes;

  // The outer walls followed by the walls of all holes. This is what the convex parts index into,
  // not the walls themselves.
  std::vector<ivec2>     surface_points;
  std::vector<IndexList> convex_parts; // Used for selection

  void get_render_data(RenderList& list);
  void recompute_lines();
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
    render_data_line->order = 66;
  }

  void get_render_data(RenderList& list)
  {
    if (render_data_line->is_valid())
    {
      list.push_back(render_data_line);
    }
  }
};

using EditorObjectRenderData = std::variant<EditorGenericRenderData, EditorSectorRenderData, EditorLineRenderData>;

}

#endif // #if NC_EDITOR
