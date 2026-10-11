// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>
#include <editor/editor_objects.h>

#include <variant>
#include <map>

namespace nc
{

struct EditorLevel;

struct ActionCreateOrDeleteLine
{
  bool        create        = true;
  bool        was_performed = false;
  EditorID    line_id;
  EditorCoord from;
  EditorCoord to;

  bool do_create(EditorLevel&   level);
  bool do_destroy(EditorLevel&  level);
  bool action_do(EditorLevel&   level); // creating a line returns true if a new sector is created
  void action_undo(EditorLevel& level);
};

struct ActionMovePoints
{
  struct LineToMove
  {
    EditorID         line_id; // delete it first then recreate it
    EditorCoord      c1;      // then recreate on these coords
    EditorCoord      c2;
    // The original sector to create if necessary
    EditorID         sector_id_to_create = INVALID_EDITOR_ID;
    EditorSectorData sector_data_to_create;
    // The other, possibly right sector
    EditorID         other_sector_id_to_create = INVALID_EDITOR_ID;
    EditorSectorData other_sector_data_to_create;
    // The destroy part
    EditorID         sector_id_to_destroy = INVALID_EDITOR_ID;
    EditorSectorData sector_data_to_destroy;
    // The other, possibly right sector to destroy
    EditorID         other_sector_id_to_destroy = INVALID_EDITOR_ID;
    EditorSectorData other_sector_data_to_destroy;
  };

  std::vector<LineToMove> lines_to_edit;
  std::vector<LineToMove> lines_to_undo;

  void init_with_new_coords
  (
    const EditorLevel& level, const std::map<EditorID, EditorCoord>& move_points
  );

  void action_do(EditorLevel&   level);
  void action_undo(EditorLevel& level);
};

using EditorAction = std::variant<ActionCreateOrDeleteLine, ActionMovePoints>;

// The action is free to return any return type
template<typename ActionType>
auto perform_action(EditorLevel& level, ActionType& action)
{
  return action.action_do(level);
}

template<typename ActionType>
void undo_action(EditorLevel& level, ActionType& action)
{
  action.action_undo(level);
}

}

#endif // #if NC_EDITOR
