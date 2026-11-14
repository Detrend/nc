// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <types.h>
#include <math/vector.h>

#include <editor/editor_types.h>
#include <editor/rendering_modifier.h>
#include <editor/editor_primitive.h>

#include <vector>

namespace nc
{

struct EditorImpl;

// Enables selection and movement of walls, sectors and entities.
struct SelectTool : public IEditorPrimitiveRenderingModifier
{
  EditorImpl&           editor;
  std::vector<EditorID> selected_objects;
  std::vector<EditorID> dragged_points;
  EditorID              pointed_at_object       = INVALID_EDITOR_ID;
  bool                  is_dragging             = false;
  vec2                  dragging_start_pt       = VEC2_ZERO;
  EditorPrimitivePtr    dragging_outline_lines  = std::make_shared<EditorPrimitive>();

  std::vector<EditorPrimitiveAndProperties> dragging_outline_points;

  bool                  is_region_selecting = false;
  vec2                  region_start_pt     = VEC2_ZERO;
  std::vector<EditorID> region_objects;
  EditorPrimitivePtr    region_outline = std::make_shared<EditorPrimitive>();
  EditorPrimitivePtr    region_surface = std::make_shared<EditorPrimitive>();

  SelectTool(EditorImpl& impl) : editor(impl) {};

  void modify_rendering_properties
  (
    EditorPrimitiveRenderingProperties& properties,
    const EditorPrimitive&              primitive
  );

  void get_render_data(RenderList& /*list*/);
  void deselect_non_existing_objects();
  void update(f32 /*dt*/);
  void get_modifiers(RenderModifierList& list);

  bool handle_dragging();
  bool handle_selection();
  void handle_region_selection(vec2 cursor_pos_in_world);
  bool handle_deletion();

  bool is_selectable(EditorID object) const;

  [[nodiscard]] EditorID find_pointed_at_object(vec2 cursor_pos_in_world) const;
};

}

#endif // #if NC_EDITOR
