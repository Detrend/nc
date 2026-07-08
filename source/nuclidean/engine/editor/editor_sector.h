// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/editor/editor_primitive.h>

#include <math/vector.h>

#include <vector>

namespace nc
{

struct EditorWall
{
  ivec2 pt;
};

struct EditorSector
{
  EditorPrimitivePtr      render_data_lines   = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr      render_data_surface = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr      render_data_splits  = std::make_shared<EditorPrimitive>();
  std::vector<EditorWall> walls;
  f32                     floor_height = 0.0f;
  f32                     ceil_height  = 0.0f;
  u64                     id;

  void get_render_data(RenderList& list);
  void recompute_lines();
  void convexify_surface();
  void recompute_render_data();
};

}
