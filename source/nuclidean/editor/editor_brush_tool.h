// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <types.h>
#include <math/vector.h>

#include <editor/editor_types.h>
#include <editor/rendering_modifier.h>
#include <editor/editor_primitive.h>

namespace nc
{

namespace editor
{

// The amount of screen space the brush cursor occupies.
constexpr f32 BRUSH_CURSOR_SCREEN_PERCENTAGE = 0.005f;

constexpr color4 BRUSH_WALL_COL1           = colors::WHITE;
constexpr color4 BRUSH_WALL_COL2           = colors::WHITE;
constexpr f32    BRUSH_WALL_FLASH_INTERVAL = 0.5f;

}

struct EditorImpl;

// Paints new walls into the level.
struct BrushTool
{
  EditorImpl&        editor;
  EditorPrimitivePtr cursor         = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr render_data    = std::make_shared<EditorPrimitive>();
  bool               is_painting    = false;
  EditorCoord        painting_start = EditorCoord{0};

  BrushTool(EditorImpl& impl) : editor(impl) {};

  void update(f32 /*delta*/);
  void get_render_data(RenderList& list);
  void get_modifiers(RenderModifierList& /*list*/);
};

}

#endif // #if NC_EDITOR
