// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <engine/editor/editor_actions.h>
#include <engine/editor/editor_level.h>

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
void ActionCreateOrDeleteLine::action_do(EditorLevel& level)
{
  was_performed = create ? this->do_create(level) : this->do_destroy(level);
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

}

#endif // #if NC_EDITOR
