// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <metaprogramming.h> // ARRAY_LENGTH
#include <types.h>
#include <math/vector.h>

#include <editor/editor_types.h>
#include <editor/rendering_modifier.h>
#include <editor/editor_primitive.h>
#include <editor/editor_renderer.h>
#include <editor/editor_level.h>
#include <editor/editor_objects_render.h>
#include <editor/editor_actions.h>

#include <map>
#include <set>
#include <vector>
#include <variant>

namespace nc
{

namespace editor
{

// Default alpha of the grid.
constexpr f32 GRID_ALPHA = 0.3f;

// Width/height of one grid cell has to occupy at least this amount of screen in order to be visible
constexpr f32 GRID_SCREEN_PERCENTAGE_FOR_VISIBILITY = 0.01f;

// The amount of screen space the brush cursor occupies.
constexpr f32 BRUSH_CURSOR_SCREEN_PERCENTAGE = 0.005f;

// The amount of screen space the normals of the wall occupy.
constexpr f32 WALL_NORMAL_SCREEN_PERCENTAGE = 0.01f;

constexpr color4 BRUSH_WALL_COL1           = colors::WHITE;
constexpr color4 BRUSH_WALL_COL2           = colors::WHITE;
constexpr f32    BRUSH_WALL_FLASH_INTERVAL = 0.5f;

// Maximum distance from center in all 4 directions.
constexpr s64 LEVEL_AREA_LIMIT = 512 * 48;
constexpr f32 ZOOM_LIMIT_MIN   = -100.0f;
constexpr f32 ZOOM_LIMIT_MAX   =  -10.0f;
constexpr f32 ZOOM_DEFAULT     =  -30.0f;

}

struct EditorImpl
{
  static constexpr u64 PX_PER_M  = 48;
  static constexpr u64 NUM_GRIDS = 5;

  static constexpr u64    GRID_SIZES[]  = {1, PX_PER_M, PX_PER_M * 8, PX_PER_M * 64, PX_PER_M * 512};
  static constexpr color4 GRID_COLORS[] =
  {
    colors::GRAY, colors::WHITE, colors::RED, colors::BLUE, colors::MAGENTA
  };

  static_assert(ARRAY_LENGTH(GRID_SIZES)  == NUM_GRIDS);
  static_assert(ARRAY_LENGTH(GRID_COLORS) == NUM_GRIDS);

  f32                              aspect = 1.0f;
  ivec2                            screen_size = ivec2{1920, 1080};
  std::vector<EditorPrimitivePtr>  grids;
  EditorRenderer                   renderer;
  vec2                             center = VEC2_ZERO;
  f32                              zoom   = editor::ZOOM_DEFAULT;
  u64                              current_snap = 1;
  f64                              time_since_start = 0.0f;
  EditorLevel                      level;

  // The object state
  std::map<EditorID, EditorObject>           objects_mirror;
  std::map<EditorID, EditorObjectRenderData> objects_render;

  // Action stack
  std::vector<EditorAction> action_history;
  std::vector<EditorAction> undone_actions;

  // Sectors whose render data has to be recomputed at the end of the current update
  std::set<EditorID> dirty_objects;

  void recompute_sector_render_data(const EditorSector& sector, EditorSectorRenderData& render_data);

  void on_object_created(EditorID object_id, const EditorSector& sector);
  void on_object_created(EditorID object_id, const EditorLine& line);
  void on_object_destroyed(EditorID object_id, const EditorLine&);
  void on_object_modified(EditorID object_id, const EditorLine&, const EditorLine& line);
  void on_object_destroyed(EditorID object_id, const EditorSector& sector);
  void on_object_modified(EditorID object_id, const EditorSector& old_state, const EditorSector& new_state);
  void on_object_modified(EditorID object_id, const EditorPoint&, const EditorPoint& point);

  void on_line_modified(EditorID line_id);

  void recompute_dirty_objects();

  void check_level_state_update();

  void update(f32 dt);

  bool try_insert_line_into_map(EditorCoord start, EditorCoord end, bool& sector_created);

  template<typename ActionType>
  auto do_action(ActionType&& action)
  {
    undone_actions.clear(); // no way to redo now
    action_history.push_back(std::move(action));

    return std::get<ActionType>(action_history.back()).action_do(level);
  }

  void undo_last_action()
  {
    if (action_history.size())
    {
      std::visit([&](auto& action)
      {
        action.action_undo(level);
      }, action_history.back());

      undone_actions.push_back(std::move(action_history.back())); // Add into the redo list
      action_history.pop_back();
    }
  }

  void redo_undone_action()
  {
    if (undone_actions.size())
    {
      action_history.push_back(std::move(undone_actions.back()));
      undone_actions.pop_back();

      std::visit([&](auto& action)
      {
        action.action_do(level);
      }, action_history.back());
    }
  }

  // Enables selection and movement of walls, sectors and entities.
  struct SelectTool : public IEditorPrimitiveRenderingModifier
  {
    std::vector<EditorID> selected_objects;
    std::vector<EditorID> dragged_points;
    EditorID              pointed_at_object       = INVALID_EDITOR_ID;
    bool                  is_dragging             = false;
    vec2                  dragging_start_pt       = VEC2_ZERO;
    EditorPrimitivePtr    dragging_outline_lines  = std::make_shared<EditorPrimitive>();
    EditorPrimitivePtr    dragging_outline_points = std::make_shared<EditorPrimitive>();

    void modify_rendering_properties
    (
      EditorPrimitiveRenderingProperties& properties,
      const EditorPrimitive&              primitive
    );

    void get_render_data(EditorImpl& impl, RenderList& /*list*/);
    void deselect_non_existing_objects(EditorImpl& impl);
    bool handle_dragging(EditorImpl& /*editor*/);
    bool handle_selection(EditorImpl& editor);
    void update(EditorImpl& editor, f32 /*dt*/);
    void get_modifiers(EditorImpl& impl, RenderModifierList& list);
  };

  struct BrushTool
  {
    EditorPrimitivePtr cursor         = std::make_shared<EditorPrimitive>();
    EditorPrimitivePtr render_data    = std::make_shared<EditorPrimitive>();
    bool               is_painting    = false;
    EditorCoord        painting_start = EditorCoord{0};

    void update(EditorImpl& editor, f32 /*delta*/);
    void get_render_data(EditorImpl& impl, RenderList& list);
    void get_modifiers(EditorImpl& impl, RenderModifierList& /*list*/);
  };

  std::variant<SelectTool, BrushTool> tool; // not the award winning one

  bool is_dragging                      = false;
  vec2 dragging_start_cursor_screen_pos = VEC2_ZERO;
  vec2 dragging_start_center_world_pos  = VEC2_ZERO;

  template<typename ToolType>
  bool has_tool_selected()
  {
    return std::holds_alternative<ToolType>(this->tool);
  }

  template<typename NewToolType>
  void change_tool()
  {
    if (this->has_tool_selected<NewToolType>())
    {
      return;
    }

    // Create the data
    this->tool = NewToolType{};
  }

  void snap_to_grid(vec2& coords);
  vec2 snap_to_grid_inplace(vec2 coords);

  vec2 get_offset() const;

  void set_offset(vec2 new_offset);

  void on_offset_changed(vec2 /*prev*/, vec2 /*curr*/);

  f32 get_zoom() const;

  void set_zoom(f32 new_zoom);

  void on_zoom_changed(f32 /*previous_zoom*/, f32 /*new_zoom*/);

  void recompute_exact_grid(EditorPrimitive& primitive, u64 step_size, color4 color);

  void recompute_grids();

  void init();

  mat3 calc_view_matrix();

  f32  get_zoom_factor() const;

  vec2 screen_to_wpos(vec2 screen_pos);

  vec2 wpos_to_screen(vec2 wpos);

  vec2 get_mouse_normalized();

  vec2 get_mouse_screen_pos();

  vec2 get_mouse_wpos();

  bool handle_keybinds();

  bool handle_dragging();

  bool handle_zoom_in_out();
};

}

#endif // #if NC_EDITOR
