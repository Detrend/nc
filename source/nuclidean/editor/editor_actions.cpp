// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_actions.h>
#include <editor/editor_level.h>

#include <algorithm>
#include <set>
#include <ranges>

namespace nc
{

//==================================================================================================
bool ActionCreateOrDeleteLine::do_create(EditorLevel& level)
{
  return level.create_line(line_id, from, to);
}

//==================================================================================================
bool ActionCreateOrDeleteLine::do_destroy(EditorLevel& level)
{
  return level.destroy_line(line_id);
}

//==================================================================================================
bool ActionCreateOrDeleteLine::action_do(EditorLevel& level)
{
  bool retval = true;

  if (create)
  {
    nc_assert(!level.get_any_object(line_id));
    was_performed = level.can_create_line(from, to);
    retval = this->do_create(level); // signal that the new sector was actually created..
  }
  else
  {
    was_performed = this->do_destroy(level);
  }

  return retval;
}

//==================================================================================================
void ActionCreateOrDeleteLine::action_undo(EditorLevel& level)
{
  // The action did nothing at all, so there is nothing to take back either
  if (!was_performed)
  {
    return;
  }

  if (create)
  {
    this->do_destroy(level);
  }
  else
  {
    this->do_create(level);
  }
}

//==================================================================================================
void ActionMovePoints::init_with_new_coords
(
  const EditorLevel& level, const std::map<EditorID, EditorCoord>& move_points
)
{
  // First accumulate the list of lines to edit
  std::map<EditorID, LineToMove> change_lines;

  for (const auto&[point_id, new_coord] : move_points)
  {
    // Collect all lines from this point that should be moved
    if (level.get_object<EditorPoint>(point_id).coords == new_coord)
    {
      // The point is not being moved, skip it
      continue;
    }

    for (EditorID half_edge_id : level.point_to_half_edges.at(point_id))
    {
      EditorID line_id = level.get_object<EditorHalfEdge>(half_edge_id).line;
      if (change_lines.contains(line_id))
      {
        // Already present, happens if two points sharing a line both move
        continue;
      }

      const EditorLine& line = level.get_object<EditorLine>(line_id);

      EditorID p1 = level.get_object<EditorHalfEdge>(line.half_edge_a).from;
      EditorID p2 = level.get_object<EditorHalfEdge>(line.half_edge_b).from;

      EditorCoord c1 = move_points.contains(p1) ? move_points.at(p1) : level.get_object<EditorPoint>(p1).coords;
      EditorCoord c2 = move_points.contains(p2) ? move_points.at(p2) : level.get_object<EditorPoint>(p2).coords;

      change_lines.insert({line_id, LineToMove{.line_id = line_id, .c1 = c1, .c2 = c2}});
    }
  }

  // Set it here, no duplicates are gonna happen
  std::transform(change_lines.begin(), change_lines.end(), std::back_inserter(this->lines_to_edit),
  [&](const auto& pair)
  {
    return pair.second;
  });
}

//==================================================================================================
void ActionMovePoints::action_do(EditorLevel& level)
{
  // First delete the lines
  for (const LineToMove& line : this->lines_to_edit)
  {
    LineToMove reverse_action;
    reverse_action.line_id = line.line_id;
    reverse_action.c1 = level.get_line_coord(line.line_id, true );
    reverse_action.c2 = level.get_line_coord(line.line_id, false);
    reverse_action.sector_id_to_create       = INVALID_EDITOR_ID;
    reverse_action.other_sector_id_to_create = INVALID_EDITOR_ID;

    auto callback_destroyed = [&](EditorID sector_id, const EditorSectorData& data)
    {
      reverse_action.sector_id_to_create   = sector_id;
      reverse_action.sector_data_to_create = data;
    };

    auto callback_merged = [&]
    (
      EditorID  left_id,  const EditorSectorData& left_data,
      EditorID  right_id, const EditorSectorData& right_data,
      EditorID& new_id, EditorSectorData&         new_data
    )
    {
      reverse_action.sector_id_to_create         = left_id;
      reverse_action.sector_data_to_create       = left_data;
      reverse_action.other_sector_id_to_create   = right_id;
      reverse_action.other_sector_data_to_create = right_data;
      // Keep the left because why not
      new_id   = left_id;
      new_data = left_data;
    };

    // Delete it
    level.destroy_line(line.line_id, callback_destroyed, callback_merged);

    this->lines_to_undo.push_back(reverse_action);
  }

  // Then create the moved versions
  for (const LineToMove& line : this->lines_to_edit)
  {
    auto callback_created = [&](EditorID& new_id, EditorSectorData& data)
    {
      // No need to do anything
      new_id = level.new_id();
      data   = EditorSectorData{};
    };

    auto callback_split = [&]
    (
      EditorID  old_id,   const EditorSectorData& old_data,
      EditorID& left_id,  EditorSectorData&       left_data,
      EditorID& right_id, EditorSectorData&       right_data
    )
    {
      // Keep the left
      left_id    = old_id;
      right_id   = level.new_id();
      left_data  = old_data;
      right_data = old_data;
    };

    // Check if there is a line between these 2 points first
    EditorID possible_line = level.get_line_between_points(line.c1, line.c2);
    if (possible_line != INVALID_EDITOR_ID)
    {
      // If there is a line then don't create other one, there can be only one line between
      // two points. So we skip the creation, but then later we restore it back during the
      // undo.
      continue;
    }

    // And then recreate it at a new position
    level.create_line(line.line_id, line.c1, line.c2, callback_created, callback_split);
  }
}

//==================================================================================================
void ActionMovePoints::action_undo(EditorLevel& level)
{
  // Revert the operations in the reverse orders they were originally performed - destructions
  for (const LineToMove& line_action : this->lines_to_undo | std::views::reverse)
  {
    auto callback_destroyed = [&](EditorID /*destroyed_id*/, const EditorSectorData& /*destroyed_data*/)
    {
      
    };

    auto callback_merged = [&]
    (
      EditorID  left_id,  const EditorSectorData& left_data,
      EditorID /*right_id*/, const EditorSectorData& /*right_data*/,
      EditorID& new_id,   EditorSectorData&       new_data
    )
    {
      new_id   = left_id;
      new_data = left_data;
    };

    if (level.try_get_object<EditorLine>(line_action.line_id))
    {
      level.destroy_line(line_action.line_id, callback_destroyed, callback_merged);
    }
  }

  // Revert the operations in the reverse orders they were originally performed - creations
  for (const LineToMove& line_action : this->lines_to_undo | std::views::reverse)
  {
    auto callback_created = [&](EditorID& new_id, EditorSectorData& new_data)
    {
      new_id   = line_action.sector_id_to_create;
      new_data = line_action.sector_data_to_create;
      nc_assert(new_id != INVALID_EDITOR_ID);
    };

    auto callback_split = [&]
    (
      EditorID /*old_id*/, const EditorSectorData& /*old_data*/,
      EditorID& id_left,  EditorSectorData&       data_left,
      EditorID& id_right, EditorSectorData&       data_right
    )
    {
      id_left    = line_action.sector_id_to_create;
      id_right   = line_action.other_sector_id_to_create;
      data_left  = line_action.sector_data_to_create;
      data_right = line_action.other_sector_data_to_create;
      nc_assert(id_left != INVALID_EDITOR_ID && id_right != INVALID_EDITOR_ID);
    };

    level.create_line
    (
      line_action.line_id, line_action.c1, line_action.c2, callback_created, callback_split
    );
  }

  // Clear the undo steps after performing the undo. If we do the redo then we fill them again.
  this->lines_to_undo.clear();
}

}

#endif // #if NC_EDITOR
