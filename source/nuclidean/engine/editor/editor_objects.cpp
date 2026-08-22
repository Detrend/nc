// Project Nuclidean Source File

#include <engine/editor/editor_objects.h>
#include <math/utils.h>    // sgn
#include <math/lingebra.h> // normalize

#include <stack_vector.h>

#include <utility>
#include <algorithm>
#include <iostream>
#include <format>
#include <cmath>

namespace nc
{

//==================================================================================================
EditorID EditorLevel::new_id() const
{
  u64 id = 0;
  do
  {
    id = __rdtsc(); // Cycle counter is indeed a neat randomness source
  }
  while (this->objects.contains(id));

  return id;
}

//==================================================================================================
bool EditorLevel::get_point_on_coord(EditorCoord coord, EditorID& id_out)
{
  for (auto&[id, object] : objects)
  {
    if (EditorPoint* point = std::get_if<EditorPoint>(&object); point && point->coords == coord)
    {
      id_out = id;
      return true;
    }
  }

  return false;
}

//==================================================================================================
void EditorLevel::on_object_created(EditorID id, const EditorPoint& point)
{
  coord_to_point.insert(std::make_pair(point.coords, id));
}

//==================================================================================================
void EditorLevel::on_object_destroyed(EditorID, const EditorPoint& point)
{
  coord_to_point.erase(point.coords);
}

//==================================================================================================
void EditorLevel::on_object_created(EditorID id, const EditorHalfEdge& half_edge)
{
  point_to_half_edges[half_edge.from].push_back(id);
}

//==================================================================================================
void EditorLevel::destroy_object(EditorID id)
{
  if (objects.contains(id))
  {
    std::visit([&]<typename T>(T& object)
    {
      if constexpr (requires {this->on_object_destroyed(id, object);})
      {
        this->on_object_destroyed(id, object);
      }
    }, objects[id]);
  }
}

//==================================================================================================
bool EditorLevel::can_create_line(EditorCoord start, EditorCoord end)
{
  if (start == end)
  {
    return false;
  }

  auto orientation = [](EditorCoord p, EditorCoord q, EditorCoord r) -> s32
  {
    s64 value = cast<s64>(q.x - p.x) * cast<s64>(r.y - p.y) - cast<s64>(q.y - p.y) * cast<s64>(r.x - p.x);
    return (value > 0) - (value < 0);
  };

  auto on_segment = [](EditorCoord p, EditorCoord q, EditorCoord r) -> bool
  {
    return std::min(p.x, r.x) <= q.x && q.x <= std::max(p.x, r.x) &&
           std::min(p.y, r.y) <= q.y && q.y <= std::max(p.y, r.y);
  };

  bool has_intersection = false;

  // Iterate all lines and check if they do not intersect with this one
  this->for_each_object_of_type<EditorLine>([&](EditorLine& line)
  {
    EditorHalfEdge& half_edge_a = this->get_object<EditorHalfEdge>(line.half_edge_a);
    EditorHalfEdge& half_edge_b = this->get_object<EditorHalfEdge>(line.half_edge_b);
    EditorCoord     other_a     = this->get_object<EditorPoint>(half_edge_a.from).coords;
    EditorCoord     other_b     = this->get_object<EditorPoint>(half_edge_b.from).coords;

    bool share_start_a = start == other_a;
    bool share_start_b = start == other_b;
    bool share_end_a   = end   == other_a;
    bool share_end_b   = end   == other_b;
    s32  shared_count  = share_start_a + share_start_b + share_end_a + share_end_b;

    // If the new segment shares exactly one endpoint with this line, ignore that shared point -
    // only flag an intersection if the two segments overlap past it, i.e. they are collinear and
    // extend beyond the shared point in the same direction.
    if (shared_count == 1)
    {
      EditorCoord shared_point    = (share_start_a || share_start_b) ? start   : end;
      EditorCoord new_far_point   = (share_start_a || share_start_b) ? end     : start;
      EditorCoord other_far_point = (share_start_a || share_end_a)   ? other_b : other_a;

      if (orientation(shared_point, new_far_point, other_far_point) != 0)
      {
        return;
      }

      s64 dot = cast<s64>(new_far_point.x - shared_point.x) * cast<s64>(other_far_point.x - shared_point.x) +
                cast<s64>(new_far_point.y - shared_point.y) * cast<s64>(other_far_point.y - shared_point.y);

      if (dot <= 0)
      {
        return;
      }

      has_intersection = true;
      return;
    }

    s32 o1 = orientation(start,   end,     other_a);
    s32 o2 = orientation(start,   end,     other_b);
    s32 o3 = orientation(other_a, other_b, start);
    s32 o4 = orientation(other_a, other_b, end);

    bool intersects = (o1 != o2 && o3 != o4)                           ||
                      (o1 == 0 && on_segment(start, other_a, end))     ||
                      (o2 == 0 && on_segment(start, other_b, end))     ||
                      (o3 == 0 && on_segment(other_a, start, other_b)) ||
                      (o4 == 0 && on_segment(other_a, end, other_b));

    if (intersects)
    {
      has_intersection = true;
    }
  });

  return !has_intersection;
}

//==================================================================================================
// Note that this is very naive and could be faster with a small amount of precalculation.
static bool is_sector_within_sector
(
  const EditorLevel& level, EID<EditorSector2> sector1_id, EID<EditorSector2> sector2_id
)
{
  // Check if all points of sector1 are inside of sector2
  const EditorSector2& sector1 = level.get_object<EditorSector2>(sector1_id);
  const EditorSector2& sector2 = level.get_object<EditorSector2>(sector2_id);

  EditorID half_edge_id = sector1.edge;
  do
  {
    const EditorHalfEdge& half_edge = level.get_object<EditorHalfEdge>(half_edge_id);
    EditorCoord point = level.get_object<EditorPoint>(half_edge.from).coords;

    // Iterate all edges of the second sector and check if this point is inside all of them
    EditorID outer_sector_edge_id = sector2.edge;
    do
    {
      const EditorHalfEdge& outer_edge      = level.get_object<EditorHalfEdge>(outer_sector_edge_id);
      const EditorHalfEdge& outer_edge_twin = level.get_object<EditorHalfEdge>(outer_edge.twin);
      EditorCoord outer_c1 = level.get_object<EditorPoint>(outer_edge.from     ).coords;
      EditorCoord outer_c2 = level.get_object<EditorPoint>(outer_edge_twin.from).coords;

      // First check if the points are not same, because then we still want to consider it as being
      // inside.
      if (point == outer_c1 || point == outer_c2)
      {
        outer_sector_edge_id = outer_edge.next;
        continue;
      }

      // Now do a determinant check and if it is positive then it means that we are on the left side.
      // Zero means we are on the line and that should never happen at all..
      EditorCoord outer_dir = outer_c2 - outer_c1;
      EditorCoord dir_to_p  = point    - outer_c1;
      s64 det = cast<s64>(outer_dir.x) * dir_to_p.y - cast<s64>(outer_dir.y) * dir_to_p.x;
      if (det > 0)
      {
        // Means we are inside! Continue with the next edge..
        outer_sector_edge_id = outer_edge.next;
        continue;
      }
      else
      {
        // The point is outside or on the boundary. We can do a quick exit..
        return false;
      }
    } while(outer_sector_edge_id != sector2.edge);

    half_edge_id = half_edge.next;
  } while(half_edge_id != sector1.edge);

  // If we got here then all points of sector1 are inside of sector2, otherwise we would do an early
  // exit.
  return true;
}

//==================================================================================================
static EditorID find_parent_sector(const EditorLevel& level, EditorID parent_id, EditorID sector_id)
{
  const EditorSector2& top = parent_id == INVALID_EDITOR_ID
    ? level.void_sector
    : level.get_object<EditorSector2>(parent_id);

  // Search through all sub-sectors
  EditorID hole_id = top.first_hole;

  if (hole_id != INVALID_EDITOR_ID)
  {
    do
    {
      if (hole_id != sector_id && is_sector_within_sector(level, sector_id, hole_id))
      {
        // Continue recursively down if possible
        EditorID sub_hole_id = find_parent_sector(level, hole_id, sector_id);
        return sub_hole_id == INVALID_EDITOR_ID ? hole_id : sub_hole_id;
      }

      // Not inside? Then try another sector..
      const auto& hole_sector = level.get_object<EditorSector2>(hole_id);
      hole_id = hole_sector.next_hole;
    } while (hole_id != top.first_hole);
  }

  // Did not find any sector we are inside of..
  return INVALID_EDITOR_ID;
}

//==================================================================================================
// Iterates all edges from the start edge until it returns to it. On crossroads always choses the
// left most edge. The twin edge is considered as a right-most one, so it has the least priority.
// Returns the list of edges that were visited and their total signed angle. If positive 360 then
// the edges form a cycle and can be enclosed in a sector. If negative then it is exterior.
// In the list of edges, the starting edge is always the last one.
void iterate_left_most_half_edges
(
  const EditorLevel&                    level,
  EID<EditorHalfEdge>                   start_edge_id,
  f32&                                  total_signed_angle_out,
  StackVector<EID<EditorHalfEdge>, 32>& enclosed_edges_out
)
{
  // We want to accumulate the angle here and then check it at the end..
  total_signed_angle_out = 0.0f;
  EID<EditorHalfEdge> curr_edge_id = start_edge_id;

  // Continue until we return to the start and calculate the signed angle. If it turns out to be
  // positive 360 then it means we created an enclosed cycle. Otherwise ignore.
  do
  {
    const EditorHalfEdge& curr_edge = level.get_object<EditorHalfEdge>(curr_edge_id);
    EditorID line_start = curr_edge.from;
    EditorID line_end   = level.get_object<EditorHalfEdge>(curr_edge.twin).from;

    EditorCoord my_edge_start = level.get_object<EditorPoint>(line_start).coords;
    EditorCoord my_edge_end   = level.get_object<EditorPoint>(line_end).coords;
    EditorCoord my_dir        = my_edge_end - my_edge_start;

    // Query all lines leaving from the curr_point.
    nc_assert(level.point_to_half_edges.contains(line_end)); // At least one must be present
    const auto& from_end_point = level.point_to_half_edges.at(line_end);
    EditorCoord edge_start = level.get_object<EditorPoint>(line_end).coords;

    f32                 best_angle = -PI;
    EID<EditorHalfEdge> best_id    = INVALID_EDITOR_ID;

    // Choose the left-most edge
    for (const EID<EditorHalfEdge>& edge_id : from_end_point)
    {
      const EditorHalfEdge& edge = level.get_object<EditorHalfEdge>(edge_id);
      EditorID    twin_point = level.get_object<EditorHalfEdge>(edge.twin).from;
      EditorCoord edge_end   = level.get_object<EditorPoint>(twin_point).coords;
      EditorCoord edge_dir   = edge_end - edge_start;

      // Now we calculate the sign and angle
      // Note that we convert the integers to floats here because doing this only in integers would
      // require computing the sum of fractions in a difficult way and I don't want to do that.

      // We calculate the sign as a determinant
      s64 the_sign = sgn(cast<s64>(my_dir.x) * edge_dir.y - cast<s64>(my_dir.y) * edge_dir.x);

      // We calculate the angle as a dot product and keep the numerator and denominator separate
      vec2 my_dir_norm        = normalize(cast<vec2>(my_dir));
      vec2 edge_dir_norm      = normalize(cast<vec2>(edge_dir));
      f32  dot_angle_unsigned = dot(my_dir_norm, edge_dir_norm);
      f32  angle_signed       = std::acos(dot_angle_unsigned) * the_sign;

      // Edge case - the reverse edge should have -180 degrees, not 180
      if (edge_end == my_edge_start)
      {
        // -180
        angle_signed = -PI;
      }

      // Keep the one with the largest angle, which is the one that turns the most to the left.
      // The backwards edge turns -180 to the left, so it is the last option.
      if (angle_signed >= best_angle)
      {
        best_angle = angle_signed;
        best_id    = edge_id;
      }
    }

    // This should be impossible, we should always choose an edge. In the worst case scenario it will
    // be the backwards edge.
    nc_assert(best_id != INVALID_EDITOR_ID);

    // Move to the next edge
    curr_edge_id = best_id;

    // Increment the total angle by the best
    total_signed_angle_out += best_angle;

    // Push back to the list so we can return to them later
    enclosed_edges_out.push_back(curr_edge_id);
  }
  while (curr_edge_id != start_edge_id); // End when we get back to the start
}

//==================================================================================================
bool EditorLevel::create_line(EditorID line_id, EditorCoord start, EditorCoord end)
{
  if (!this->can_create_line(start, end))
  {
    return false;
  }

  nc_assert(this->get_any_object(line_id) == nullptr);

  /*[[indeterminate]]*/
  EditorID pt1_id, pt2_id;

  if (!this->get_point_on_coord(start, pt1_id))
  {
    this->create_object<EditorPoint>(pt1_id = this->new_id(), start);
  }

  if (!this->get_point_on_coord(end, pt2_id))
  {
    this->create_object<EditorPoint>(pt2_id = this->new_id(), end);
  }

  // Now that we have both points we can create a line between them and also the 2 edges
  EditorID h1_id = this->new_id(), h2_id = this->new_id();

  // Create the objects
  // TODO: This can be done using one function only
  this->create_object<EditorHalfEdge>(h1_id,   /*from*/ pt1_id, /*twin*/ h2_id);
  this->create_object<EditorHalfEdge>(h2_id,   /*from*/ pt2_id, /*twin*/ h1_id);
  this->create_object<EditorLine>    (line_id, /*edge1*/h1_id,  /*edge2*/h2_id);

  // We want to accumulate the angle here and then check it at the end..
  f32 left_signed_angle = 0.0f, right_signed_angle = 0.0f;
  StackVector<EID<EditorHalfEdge>, 32> left_enclosed_edges, right_enclosed_edges;
  iterate_left_most_half_edges(*this, h1_id, left_signed_angle, left_enclosed_edges);

  // Now that we returned back we should have a list of points through which we travelled
  // Check if the area is enclosed by examining the total signed angle
  if (!is_zero(left_signed_angle - PI2, 0.1f))
  {
    // This means that we haven't created any new sector or split another sector into 2..
    // Bail out.
    // The line was however created succesfully, so return success.
    return true;
  }

  // If the sum is ~360 degrees then the area forms a cycle! This means that we either created a
  // brand new sector or split one sector into 2.
  nc_assert(left_enclosed_edges.size() >= 3);

  auto resolve_edges_to_new_sector = [this](const StackVector<EID<EditorHalfEdge>, 32>& edges, EID<EditorSector2> new_sector_id)
  {
    EID<EditorHalfEdge> first_left_edge = edges.front();

    // This means that we split another sector into 2 parts.
    // The left one will keep the original sector.
    for (u64 edge_id_idx = 0; edge_id_idx < edges.size(); ++edge_id_idx)
    {
      EID<EditorHalfEdge> edge_id      = edges[edge_id_idx];
      EID<EditorHalfEdge> next_edge_id = edges[(edge_id_idx + 1) % edges.size()];

      EditorHalfEdge& edge = this->get_object<EditorHalfEdge>(edge_id);
      edge.sector = new_sector_id;
      edge.next   = next_edge_id;
    }

    // Change the sector's first edge to this one
    this->get_object<EditorSector2>(new_sector_id).edge = first_left_edge;
  };

  // Was this previously a sector that we split, or is it a new one? We can find out by inspecting
  // the first edge.
  EID<EditorSector2> previous_sector_id = this->get_object<EditorHalfEdge>(left_enclosed_edges.front()).sector;
  EID<EditorSector2> new_sector_id      = this->new_id();
  EditorSector2&     new_sector         = this->create_object<EditorSector2>(new_sector_id);

  if (previous_sector_id != INVALID_EDITOR_ID)
  {
    // The sector was split into 2 parts..

    iterate_left_most_half_edges(*this, h2_id, right_signed_angle, right_enclosed_edges);
    nc_assert(is_zero(right_signed_angle - PI2, 0.1f)); // This must hold if it was a sector previously

    resolve_edges_to_new_sector(left_enclosed_edges,  previous_sector_id);
    resolve_edges_to_new_sector(right_enclosed_edges, new_sector_id     );

    EditorSector2& prev_sector = this->get_object<EditorSector2>(previous_sector_id);

    // The parent has to be the same as for the previous sector obviously, because we just split
    // the old sector into 2 new ones.
    new_sector.parent     = prev_sector.parent;
    new_sector.first_hole = INVALID_EDITOR_ID; // Preventively set it here, might get changed later

    // We need to redistribute previous hole sectors into the 2 sectors - one new one and one old
    // one. We have to do this before we patch the hole list
    if (prev_sector.first_hole != INVALID_EDITOR_ID)
    {
      EID<EditorSector2> hole_rover = prev_sector.first_hole;
      StackVector<EditorID, 16> new_sector_holes, old_sector_holes;

      // Iterate all old sector holes and decide if they should become holes of the old sector or
      // the new one. Collect them into 2 lists that we will later use to connect their pointers.
      do
      {
        EditorSector2& hole = this->get_object<EditorSector2>(hole_rover);
        if (is_sector_within_sector(*this, hole_rover, new_sector_id))
        {
          hole.parent = new_sector_id;
          new_sector_holes.push_back(hole_rover);
        }
        else
        {
          old_sector_holes.push_back(hole_rover);
        }
        hole_rover = hole.next_hole;
      } while (hole_rover != prev_sector.first_hole);

      auto connect_holes = [this](const StackVector<EditorID, 16>& holes)
      {
        for (u64 i_curr = 0; i_curr < holes.size(); ++i_curr)
        {
          u64 i_next = (i_curr+1) % holes.size();
          this->get_object<EditorSector2>(holes[i_curr]).next_hole = holes[i_next];
        }
      };

      // And now iterate both lists and connect them appropriately
      connect_holes(new_sector_holes);
      connect_holes(old_sector_holes);

      // And appoint the correct first hole to both new and old sectors
      prev_sector.first_hole = old_sector_holes.size() ? old_sector_holes.front() : INVALID_EDITOR_ID;
      new_sector.first_hole  = new_sector_holes.size() ? new_sector_holes.front() : INVALID_EDITOR_ID;
    }

    // Now we need to fix the hole-list
    new_sector.next_hole  = prev_sector.next_hole;
    prev_sector.next_hole = new_sector_id;
  }
  else
  {
    // This means that we created a brand new sector.
    resolve_edges_to_new_sector(left_enclosed_edges, new_sector_id);

    // Find correct parent
    new_sector.parent = find_parent_sector(*this, INVALID_EDITOR_ID, new_sector_id);
    EditorSector2& parent = new_sector.parent == INVALID_EDITOR_ID
      ? this->void_sector
      : this->get_object<EditorSector2>(new_sector.parent);

    // Set ourselves as the first hole
    if (parent.first_hole == INVALID_EDITOR_ID)
    {
      parent.first_hole    = new_sector_id;
      new_sector.next_hole = new_sector_id;
    }
    else
    {
      // Insert after the first one
      EditorSector2& first_hole = this->get_object<EditorSector2>(parent.first_hole);
      new_sector.next_hole = first_hole.next_hole;
      first_hole.next_hole = new_sector_id;
    }
  }

  // Success, we created a new sector..
  return true;
}

//==================================================================================================
EditorObject* EditorLevel::get_any_object(EditorID any_id)
{
  if (!this->objects.contains(any_id))
  {
    return nullptr;
  }

  return &this->objects.at(any_id);
}

//==================================================================================================
void EditorLevel::destroy_line(EditorID /*edge_id*/)
{
  
}

//==================================================================================================
void dump_sector_and_subsectors(const EditorLevel& level, EditorID sector_id, s32 indent)
{
  const EditorSector2& sector = sector_id == INVALID_EDITOR_ID
    ? level.void_sector
    : level.get_object<EditorSector2>(sector_id);

  for (s32 i = 0; i < indent; ++i) std::cout << " ";

  // Iterate all half edges
  EditorID rover_edge = sector.edge;
  if (rover_edge != INVALID_EDITOR_ID)
  {
    bool first = true;
    do
    {
      auto& half_edge = level.get_object<EditorHalfEdge>(rover_edge);
      EditorCoord coord = level.get_object<EditorPoint>(half_edge.from).coords;
      std::cout << std::format("{}[{},{}]", first ? "" : ",", coord.x, coord.y);
      first = false;
      rover_edge = half_edge.next;
    } while (rover_edge != sector.edge);

    std::cout << std::endl;
  }


  // Iterate all holes
  EditorID rover_hole = sector.first_hole;
  if (rover_hole != INVALID_EDITOR_ID)
  {
    do
    {
      dump_sector_and_subsectors(level, rover_hole, indent + 2);
      rover_hole = level.get_object<EditorSector2>(rover_hole).next_hole;
    } while (rover_hole != sector.first_hole);
  }
}

//==================================================================================================
void EditorLevel::dump_to_text()
{
  std::cout << "=========================================================" << std::endl;
  std::cout << "Lines" << std::endl;

  this->for_each_object_of_type<EditorLine>([&](const EditorLine& line)
  {
    auto& edge1 = this->get_object<EditorHalfEdge>(line.half_edge_a);
    auto& edge2 = this->get_object<EditorHalfEdge>(line.half_edge_b);
    auto& pt1   = this->get_object<EditorPoint>(edge1.from).coords;
    auto& pt2   = this->get_object<EditorPoint>(edge2.from).coords;
    std::cout << std::format("[{},{}], [{},{}]", pt1.x, pt1.y, pt2.x, pt2.y) << std::endl;
  });

  std::cout << "Sectors" << std::endl;
  dump_sector_and_subsectors(*this, INVALID_EDITOR_ID, 0);
}

//==================================================================================================
void test_editor_level()
{
  EditorLevel level;

  ivec2 pt1 = ivec2{1,  1};
  ivec2 pt2 = ivec2{4,  4};
  ivec2 pt3 = ivec2{6,  5};
  ivec2 pt4 = ivec2{-3, 2};

  level.create_line(level.new_id(), pt1, pt2);
  level.dump_to_text();
  level.create_line(level.new_id(), pt2, pt3);
  level.dump_to_text();
  level.create_line(level.new_id(), pt3, pt4);
  level.dump_to_text();
  level.create_line(level.new_id(), pt4, pt1);
  level.dump_to_text();
  level.create_line(level.new_id(), pt2, pt4);
  level.dump_to_text();
  exit(69);
}

//==================================================================================================
void test_editor_level_circle_split()
{
  EditorLevel level;

  s32 point_count = 12;
  f32 radius      = 10.0f;

  std::vector<ivec2> points;
  for (s32 i = 0; i < point_count; ++i)
  {
    f32 angle = PI2 * cast<f32>(i) / cast<f32>(point_count);
    s32 x = cast<s32>(std::round(radius * std::cos(angle)));
    s32 y = cast<s32>(std::round(radius * std::sin(angle)));
    points.push_back(ivec2{x, y});
  }

  // Create the boundary lines forming a single circular sector.
  for (s32 i = 0; i < point_count; ++i)
  {
    level.create_line(level.new_id(), points[i], points[(i + 1) % point_count]);
  }
  level.dump_to_text();

  // Split the circular sector into a central square and 4 outer sectors by connecting every 3rd point.
  for (s32 i = 0; i < point_count; i += 3)
  {
    level.create_line(level.new_id(), points[i], points[(i + 3) % point_count]);
    level.dump_to_text();
  }

  exit(69);
}

//==================================================================================================
void build_map_from_actions(std::vector<EditorAction>& actions)
{
  EditorLevel level;

  for (auto& action : actions)
  {
    std::visit([&]<typename T>(T& action)
    {
      perform_action<T>(level, action);
    }, action);

    level.dump_to_text();
  }
}

//==================================================================================================
void test_editor_build_from_actions()
{
  std::vector<EditorAction> actions;

  EditorID next_id = 1;

  auto add_line_action = [&](ivec2 from, ivec2 to)
  {
    ActionCreateOrDeleteLine action;
    action.create  = true;
    action.line_id = next_id++;
    action.from    = from;
    action.to      = to;
    actions.push_back(action);
  };

  s32 point_count = 12;
  f32 radius      = 10.0f;

  std::vector<ivec2> points;
  for (s32 i = 0; i < point_count; ++i)
  {
    f32 angle = PI2 * cast<f32>(i) / cast<f32>(point_count);
    s32 x = cast<s32>(std::round(radius * std::cos(angle)));
    s32 y = cast<s32>(std::round(radius * std::sin(angle)));
    points.push_back(ivec2{x, y});
  }

  // Boundary lines forming a single circular sector.
  for (s32 i = 0; i < point_count; ++i)
  {
    add_line_action(points[i], points[(i + 1) % point_count]);
  }

  // A small square, fully disjoint from the circle boundary. Closing its loop creates a brand
  // new sector nested entirely inside the circle, exercising find_parent_sector.
  s32 square_point_count = 4;
  std::vector<ivec2> square_points =
  {
    ivec2{ 3,  3},
    ivec2{-3,  3},
    ivec2{-3, -3},
    ivec2{ 3, -3},
  };

  for (s32 i = 0; i < square_point_count; ++i)
  {
    add_line_action(square_points[i], square_points[(i + 1) % square_point_count]);
  }

  // Division lines connecting each square corner to a nearby circle point. These split the ring
  // between the circle and the square into multiple sectors, exercising redistribution of the
  // square hole into whichever split sector ends up actually containing it.
  add_line_action(square_points[0], points[1]);
  add_line_action(square_points[1], points[4]);
  add_line_action(square_points[2], points[7]);
  add_line_action(square_points[3], points[10]);

  build_map_from_actions(actions);
  exit(67);
}

//==================================================================================================
static bool shit = []()
{
  //test_editor_level_circle_split();
  test_editor_build_from_actions();
  return true;
}();

//==================================================================================================
bool ActionCreateOrDeleteLine::do_create(EditorLevel& level)
{
  return level.create_line(line_id, from, to);
}

//==================================================================================================
void ActionCreateOrDeleteLine::do_destroy(EditorLevel& level)
{
  level.destroy_line(line_id);
}

//==================================================================================================
void ActionCreateOrDeleteLine::action_do(EditorLevel& level)
{
  if (create)
  {
    was_performed = this->do_create(level);
  }
  else
  {
    this->do_destroy(level);
  }
}

//==================================================================================================
void ActionCreateOrDeleteLine::action_undo(EditorLevel& level)
{
  if (!create)
  {
    this->do_create(level);
  }
  else
  {
    if (was_performed)
    {
      this->do_destroy(level);
    }
  }
}

}
