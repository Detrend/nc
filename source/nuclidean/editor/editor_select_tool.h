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
  bool handle_deletion();
};

}

#endif // #if NC_EDITOR
