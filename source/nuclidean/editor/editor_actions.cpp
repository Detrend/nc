// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_actions.h>
#include <editor/editor_level.h>

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
  const EditorLevel& level, std::map<EditorID, EditorCoord>&& move_points
)
{
  // Remember the new coords
  new_coords = std::move(move_points);

  // And store the old coords for undo
  for (const auto&[point_id, _] : new_coords)
  {
    old_coords[point_id] = level.get_object<EditorPoint>(point_id).coords;
  }
}

//==================================================================================================
void ActionMovePoints::action_do(EditorLevel& level)
{
  level.move_points(new_coords);
}

//==================================================================================================
void ActionMovePoints::action_undo(EditorLevel& level)
{
  level.move_points(old_coords);
}

}

#endif // #if NC_EDITOR
