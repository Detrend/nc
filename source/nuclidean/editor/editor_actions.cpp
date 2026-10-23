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
  created_sector_id = INVALID_EDITOR_ID;

  auto callback_created = [&](EditorID& new_id, EditorSectorData& new_data)
  {
    if (sector_id_b != INVALID_EDITOR_ID && !level.get_any_object(sector_id_b))
    {
      new_id   = sector_id_b;
      new_data = sector_data_b;
    }
    else if (sector_id_a != INVALID_EDITOR_ID && !level.get_any_object(sector_id_a))
    {
      new_id   = sector_id_a;
      new_data = sector_data_a;
    }
    else
    {
      new_id   = level.new_id();
      new_data = EditorSectorData{};
    }

    created_sector_id = new_id;
  };

  auto callback_split = [&]
  (
    EditorID  old_id,  const EditorSectorData& old_data,
    EditorID& left_id, EditorSectorData&       left_data,
    EditorID& right_id, EditorSectorData&      right_data
  )
  {
    if (sector_id_a != INVALID_EDITOR_ID)
    {
      // Split it back into the two sectors that got merged when the line was destroyed
      nc_assert(sector_id_b != INVALID_EDITOR_ID); // Both filled out or either only one
      left_id    = sector_id_a;
      left_data  = sector_data_a;
      right_id   = sector_id_b;
      right_data = sector_data_b;
    }
    else
    {
      // Keep the left
      left_id    = old_id;
      left_data  = old_data;
      right_id   = level.new_id();
      right_data = old_data;
    }

    created_sector_id = left_id == old_id ? right_id : left_id;
  };

  // Bring back the points that died together with the line when it was destroyed
  auto callback_point = [&](EditorCoord coord, EditorID& id_out)
  {
    EditorID wanted_id = coord == from ? point_from : point_to;
    if (wanted_id != INVALID_EDITOR_ID && !level.get_any_object(wanted_id))
    {
      id_out = wanted_id;
    }
  };

  auto unused_or_invalid = [&](EditorID id)
  {
    return level.get_any_object(id) ? INVALID_EDITOR_ID : id;
  };

  return level.create_line
  (
    line_id, from, to, callback_created, callback_split, callback_point,
    unused_or_invalid(half_edge_a), unused_or_invalid(half_edge_b)
  );
}

//==================================================================================================
bool ActionCreateOrDeleteLine::do_destroy(EditorLevel& level)
{
  // Reset first
  sector_id_a = INVALID_EDITOR_ID;
  sector_id_b = INVALID_EDITOR_ID;

  // Remember the rest of the objects of the line, so that we can recreate it with them later
  if (const EditorLine* line = level.try_get_object<EditorLine>(line_id))
  {
    half_edge_a = line->half_edge_a;
    half_edge_b = line->half_edge_b;
    point_from  = level.get_object<EditorHalfEdge>(half_edge_a).from;
    point_to    = level.get_object<EditorHalfEdge>(half_edge_b).from;
    from        = level.get_object<EditorPoint>(point_from).coords;
    to          = level.get_object<EditorPoint>(point_to).coords;
  }

  auto callback_destroyed = [&](EditorID sector_id, const EditorSectorData& data)
  {
    sector_id_a   = sector_id;
    sector_data_a = data;
  };

  auto callback_merged = [&]
  (
    EditorID  left_id,  const EditorSectorData& left_data,
    EditorID  right_id, const EditorSectorData& right_data,
    EditorID& new_id,   EditorSectorData&       new_data
  )
  {
    sector_id_a   = left_id;
    sector_data_a = left_data;
    sector_id_b   = right_id;
    sector_data_b = right_data;

    // Keep the sector that existed before the line was created, if we know which one it was
    bool keep_right = left_id == created_sector_id;
    new_id   = keep_right ? right_id   : left_id;
    new_data = keep_right ? right_data : left_data;
  };

  return level.destroy_line(line_id, callback_destroyed, callback_merged);
}

//==================================================================================================
bool ActionCreateOrDeleteLine::action_do(EditorLevel& level)
{
  bool retval = true;

  if (create)
  {
    nc_assert(!level.get_any_object(line_id));
    was_performed = level.can_create_line(from, to);
    retval = was_performed && this->do_create(level); // signal that the new sector was actually created..
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
void ActionDeleteObjects::init_destruction
(
  const EditorLevel& level, const std::vector<EditorID>& objects_to_destroy
)
{
  // Collect a list of lines..
  std::set<EditorID> list_of_lines;

  for (EditorID object_to_destroy_id : objects_to_destroy)
  {
    if (level.try_get_object<EditorLine>(object_to_destroy_id))
    {
      // Destroy the line itself..
      list_of_lines.insert(object_to_destroy_id);
    }
    else if (const EditorPoint* point = level.try_get_object<EditorPoint>(object_to_destroy_id))
    {
      // Destroy all lines going from the point..
      for (EditorID half_edge_id : level.point_to_half_edges.at(object_to_destroy_id))
      {
        list_of_lines.insert(level.get_object<EditorHalfEdge>(half_edge_id).line);
      }
    }
    else if (const EditorSector* sector = level.try_get_object<EditorSector>(object_to_destroy_id))
    {
      // Erase only the lines that are not shared with other sector
      EditorID half_edge_id = sector->edge;
      do
      {
        const EditorHalfEdge& half_edge = level.get_object<EditorHalfEdge>(half_edge_id);
        const EditorHalfEdge& twin_edge = level.get_object<EditorHalfEdge>(half_edge.twin);

        if (twin_edge.sector == INVALID_EDITOR_ID || twin_edge.sector == object_to_destroy_id)
        {
          // Delete this one, no other sector sharing it..
          list_of_lines.insert(half_edge.line);
        }

        half_edge_id = half_edge.next; // Continue to the next one
      }
      while (half_edge_id != sector->edge);
    }
  }

  // Transform it into a normal list
  for (EditorID line_to_destroy_id : list_of_lines)
  {
    this->actions.push_back(ActionCreateOrDeleteLine{.create = false, .line_id = line_to_destroy_id});
  }
}

//==================================================================================================
void ActionDeleteObjects::action_do(EditorLevel& level)
{
  for (ActionCreateOrDeleteLine& action : this->actions)
  {
    action.action_do(level);
  }
}

//==================================================================================================
void ActionDeleteObjects::action_undo(EditorLevel& level)
{
  // Go from the back, reverse order than the one we executed them before
  for (ActionCreateOrDeleteLine& action : this->actions | std::views::reverse)
  {
    action.action_undo(level);
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
// Recreates the recorded lines in the reverse order of their destruction. This mirrors the
// destruction, so the sectors it killed or merged come back with their original IDs and data. The
// move can change the topology (points or lines merging, sectors ending up inside of others) and
// break the mirror, in which case we hand out unused recorded IDs and only then brand new ones.
static void recreate_recorded_lines
(
  EditorLevel&                                     level,
  std::vector<ActionMovePoints::LineRecord>&       records,
  const std::unordered_map<EditorCoord, EditorID>& point_ids,
  bool                                             use_new_coords
)
{
  // Returns the ID back if no object uses it at the moment, an invalid one otherwise
  auto unused_or_invalid = [&](EditorID id)
  {
    return level.get_any_object(id) ? INVALID_EDITOR_ID : id;
  };

  // Looks for any sector that the move destroyed and that has not been brought back yet
  auto find_any_unused_sector = [&](EditorID& id_out, EditorSectorData& data_out) -> bool
  {
    for (const ActionMovePoints::LineRecord& record : records)
    {
      if (unused_or_invalid(record.sector_id_a))
      {
        id_out   = record.sector_id_a;
        data_out = record.sector_data_a;
        return true;
      }

      if (unused_or_invalid(record.sector_id_b))
      {
        id_out   = record.sector_id_b;
        data_out = record.sector_data_b;
        return true;
      }
    }

    return false;
  };

  auto callback_point = [&](EditorCoord coord, EditorID& id_out)
  {
    auto it = point_ids.find(coord);
    if (it != point_ids.end())
    {
      id_out = unused_or_invalid(it->second);
    }
  };

  for (ActionMovePoints::LineRecord& record : records | std::views::reverse)
  {
    EditorCoord c1 = use_new_coords ? record.new_c1 : record.old_c1;
    EditorCoord c2 = use_new_coords ? record.new_c2 : record.old_c2;

    record.created_sector_id = INVALID_EDITOR_ID;

    // There can be only one line between two points. If the moved line ended up on top of another
    // one then we skip it and the undo brings it back.
    if (level.get_line_between_points(c1, c2) != INVALID_EDITOR_ID)
    {
      continue;
    }

    auto callback_created = [&](EditorID& new_id, EditorSectorData& new_data)
    {
      if (unused_or_invalid(record.sector_id_a))
      {
        new_id   = record.sector_id_a;
        new_data = record.sector_data_a;
      }
      else if (unused_or_invalid(record.sector_id_b))
      {
        new_id   = record.sector_id_b;
        new_data = record.sector_data_b;
      }
      else if (!find_any_unused_sector(new_id, new_data))
      {
        new_id   = level.new_id();
        new_data = EditorSectorData{};
      }

      record.created_sector_id = new_id;
    };

    auto callback_split = [&]
    (
      EditorID  old_id,   const EditorSectorData& old_data,
      EditorID& left_id,  EditorSectorData&       left_data,
      EditorID& right_id, EditorSectorData&       right_data
    )
    {
      bool mirrors_merge = (old_id == record.sector_id_a && unused_or_invalid(record.sector_id_b)) ||
                           (old_id == record.sector_id_b && unused_or_invalid(record.sector_id_a));

      if (mirrors_merge)
      {
        // Split it back into the two sectors that got merged when the line was destroyed
        left_id    = record.sector_id_a;
        left_data  = record.sector_data_a;
        right_id   = record.sector_id_b;
        right_data = record.sector_data_b;
      }
      else
      {
        // Keep the left and come up with something for the right
        left_id   = old_id;
        left_data = old_data;

        if (unused_or_invalid(record.sector_id_a))
        {
          right_id   = record.sector_id_a;
          right_data = record.sector_data_a;
        }
        else if (unused_or_invalid(record.sector_id_b))
        {
          right_id   = record.sector_id_b;
          right_data = record.sector_data_b;
        }
        else if (!find_any_unused_sector(right_id, right_data))
        {
          right_id   = level.new_id();
          right_data = old_data;
        }
      }

      record.created_sector_id = left_id == old_id ? right_id : left_id;
    };

    level.create_line
    (
      record.line_id, c1, c2, callback_created, callback_split, callback_point,
      unused_or_invalid(record.half_edge_a), unused_or_invalid(record.half_edge_b)
    );
  }
}

//==================================================================================================
void ActionMovePoints::action_do(EditorLevel& level)
{
  // Start from scratch, the redo does the very same thing as the first time
  this->line_records.clear();
  this->old_point_ids.clear();
  this->new_point_ids.clear();

  // Remember everything about the lines before we start destroying them
  for (const LineToMove& line : this->lines_to_edit)
  {
    const EditorLine& level_line = level.get_object<EditorLine>(line.line_id);

    LineRecord& record = this->line_records.emplace_back();
    record.line_id     = line.line_id;
    record.half_edge_a = level_line.half_edge_a;
    record.half_edge_b = level_line.half_edge_b;
    record.old_c1      = level.get_line_coord(line.line_id, true );
    record.old_c2      = level.get_line_coord(line.line_id, false);
    record.new_c1      = line.c1;
    record.new_c2      = line.c2;

    this->old_point_ids[record.old_c1] = level.get_object<EditorHalfEdge>(record.half_edge_a).from;
    this->old_point_ids[record.old_c2] = level.get_object<EditorHalfEdge>(record.half_edge_b).from;
  }

  // The points that stay in place come first. If a moved point lands on one of them then the two
  // merge and the one that was standing there keeps its ID.
  for (bool moved_points : {false, true})
  {
    for (const LineRecord& record : this->line_records)
    {
      auto add_point = [&](EditorCoord old_coord, EditorCoord new_coord)
      {
        if ((old_coord != new_coord) == moved_points)
        {
          this->new_point_ids.try_emplace(new_coord, this->old_point_ids.at(old_coord));
        }
      };

      add_point(record.old_c1, record.new_c1);
      add_point(record.old_c2, record.new_c2);
    }
  }

  // First delete the lines
  for (LineRecord& record : this->line_records)
  {
    auto callback_destroyed = [&](EditorID sector_id, const EditorSectorData& data)
    {
      record.sector_id_a   = sector_id;
      record.sector_data_a = data;
    };

    auto callback_merged = [&]
    (
      EditorID  left_id,  const EditorSectorData& left_data,
      EditorID  right_id, const EditorSectorData& right_data,
      EditorID& new_id,   EditorSectorData&       new_data
    )
    {
      record.sector_id_a   = left_id;
      record.sector_data_a = left_data;
      record.sector_id_b   = right_id;
      record.sector_data_b = right_data;
      // Keep the left because why not
      new_id   = left_id;
      new_data = left_data;
    };

    level.destroy_line(record.line_id, callback_destroyed, callback_merged);
  }

  // And then recreate them at the new position
  recreate_recorded_lines(level, this->line_records, this->new_point_ids, true);
}

//==================================================================================================
void ActionMovePoints::action_undo(EditorLevel& level)
{
  // Destroy the moved lines in the reverse order of their recreation, which mirrors it
  for (const LineRecord& record : this->line_records)
  {
    if (!level.try_get_object<EditorLine>(record.line_id))
    {
      // This can happen if the new line was never created because it was merged with another line
      // that had the exact same corner points.
      continue;
    }

    auto callback_merged = [&]
    (
      EditorID  left_id,  const EditorSectorData& left_data,
      EditorID  right_id, const EditorSectorData& right_data,
      EditorID& new_id,   EditorSectorData&       new_data
    )
    {
      // Keep the sector that existed before the line was recreated
      bool keep_right = left_id == record.created_sector_id;
      new_id   = keep_right ? right_id   : left_id;
      new_data = keep_right ? right_data : left_data;
    };

    level.destroy_line(record.line_id, nullptr, callback_merged);
  }

  // And recreate them on the original place
  recreate_recorded_lines(level, this->line_records, this->old_point_ids, false);
}

}

#endif // #if NC_EDITOR
