// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>
#include <editor/editor_objects.h>

#include <variant>
#include <map>
#include <unordered_map>
#include <vector>

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

  EditorID         sector_id_a = INVALID_EDITOR_ID;
  EditorSectorData sector_data_a;
  EditorID         sector_id_b = INVALID_EDITOR_ID;
  EditorSectorData sector_data_b;
  EditorID         created_sector_id = INVALID_EDITOR_ID;

  // Remembered when destroying the line, so that recreating it brings back the same objects
  EditorID half_edge_a = INVALID_EDITOR_ID;
  EditorID half_edge_b = INVALID_EDITOR_ID;
  EditorID point_from  = INVALID_EDITOR_ID;
  EditorID point_to    = INVALID_EDITOR_ID;

  bool do_create(EditorLevel&   level);
  bool do_destroy(EditorLevel&  level);
  bool action_do(EditorLevel&   level); // creating a line returns true if a new sector is created
  void action_undo(EditorLevel& level);
};

struct ActionMovePoints
{
  struct LineToMove
  {
    EditorID    line_id; // delete it first then recreate it
    EditorCoord c1;      // then recreate on these coords
    EditorCoord c2;
  };

  // Everything we need to recreate a line with the very same objects it had before. The lines are
  // destroyed in one order and recreated in the reverse one, which mirrors the destruction - the
  // last line to be recreated closes the sector loop that the first destroyed line opened.
  struct LineRecord
  {
    EditorID    line_id;
    EditorID    half_edge_a = INVALID_EDITOR_ID;
    EditorID    half_edge_b = INVALID_EDITOR_ID;
    EditorCoord old_c1, old_c2; // before the move
    EditorCoord new_c1, new_c2; // after the move
    // Sectors the destruction of the line killed (only A) or merged together (A and B)
    EditorID         sector_id_a = INVALID_EDITOR_ID;
    EditorSectorData sector_data_a;
    EditorID         sector_id_b = INVALID_EDITOR_ID;
    EditorSectorData sector_data_b;
    // Sector that recreating the line on the new place created, the undo has to get rid of it
    EditorID         created_sector_id = INVALID_EDITOR_ID;
  };

  std::vector<LineToMove> lines_to_edit;
  std::vector<LineRecord> line_records; // in the order of the destruction

  // Point IDs on their coordinates before and after the move
  std::unordered_map<EditorCoord, EditorID> old_point_ids;
  std::unordered_map<EditorCoord, EditorID> new_point_ids;

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
