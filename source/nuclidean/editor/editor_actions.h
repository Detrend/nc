// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>

#include <variant>

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

  bool do_create(EditorLevel& level);
  bool do_destroy(EditorLevel& level);
  void action_do(EditorLevel& level);
  void action_undo(EditorLevel& level);
};

using EditorAction = std::variant<ActionCreateOrDeleteLine>;

template<typename ActionType>
void perform_action(EditorLevel& level, ActionType& action)
{
  action.action_do(level);
}

template<typename ActionType>
void undo_action(EditorLevel& level, ActionType& action)
{
  action.action_undo(level);
}

}

#endif // #if NC_EDITOR
