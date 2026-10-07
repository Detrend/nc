// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>

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
  std::map<EditorID, EditorCoord> new_coords;
  std::map<EditorID, EditorCoord> old_coords;

  void init_with_new_coords
  (
    const EditorLevel& level, std::map<EditorID, EditorCoord>&& move_points
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
