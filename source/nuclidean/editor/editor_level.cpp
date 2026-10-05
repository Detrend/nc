// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_level.h>
#include <editor/editor_actions.h>

#include <stack_vector.h>
#include <math/vector.h>
#include <math/lingebra.h>
#include <math/utils.h>

#include <algorithm>     // std::find, std::remove, std::min, std::max
#include <unordered_set> // std::unordered_set
#include <intrin.h>  // __rdtsc
#include <cmath>     // std::round, std::cos, std::sin, std::acos


namespace nc
{

//==================================================================================================
EditorLevel::EditorLevel()
{
  // The void sector has to be manually created on the start..
  this->create_object<EditorSector>(VOID_SECTOR_ID);
}

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
void EditorLevel::on_object_destroyed(EditorID id, const EditorHalfEdge& half_edge)
{
  auto it = point_to_half_edges.find(half_edge.from);
  if (it == point_to_half_edges.end())
  {
    return;
  }

  // Erase without reordering the rest, so that undoing a couple of line creations gets us back to
  // the very same list we had before we created them.
  auto& edges = it->second;
  edges.erase(std::remove(edges.begin(), edges.end(), id), edges.end());

  // No edge starts in this point anymore. Before the first one was created there was no entry for
  // the point at all, so drop the whole thing instead of leaving an empty list behind.
  if (edges.empty())
  {
    point_to_half_edges.erase(it);
  }
}

//==================================================================================================
void EditorLevel::destroy_object(EditorID id)
{
  // The void surrounds everything and has to exist for as long as the level does
  nc_assert(id != VOID_SECTOR_ID);

  auto it = objects.find(id);
  if (it == objects.end())
  {
    return;
  }

  std::visit([&]<typename T>(T& object)
  {
    if constexpr (requires {this->on_object_destroyed(id, object);})
    {
      this->on_object_destroyed(id, object);
    }
  }, it->second);

  objects.erase(it);
}

//==================================================================================================
// Checks if the segment start-end intersects the segment other_a-other_b. Touching in exactly one
// shared endpoint is fine, everything else (crossing, touching in the middle, overlapping or being
// the very same segment) counts as an intersection.
static bool segments_intersect
(
  EditorCoord start, EditorCoord end, EditorCoord other_a, EditorCoord other_b
)
{
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
      return false;
    }

    s64 dot = cast<s64>(new_far_point.x - shared_point.x) * cast<s64>(other_far_point.x - shared_point.x) +
              cast<s64>(new_far_point.y - shared_point.y) * cast<s64>(other_far_point.y - shared_point.y);

    return dot > 0;
  }

  s32 o1 = orientation(start,   end,     other_a);
  s32 o2 = orientation(start,   end,     other_b);
  s32 o3 = orientation(other_a, other_b, start);
  s32 o4 = orientation(other_a, other_b, end);

  return (o1 != o2 && o3 != o4)                           ||
         (o1 == 0 && on_segment(start, other_a, end))     ||
         (o2 == 0 && on_segment(start, other_b, end))     ||
         (o3 == 0 && on_segment(other_a, start, other_b)) ||
         (o4 == 0 && on_segment(other_a, end, other_b));
}

//==================================================================================================
bool EditorLevel::can_create_line(EditorCoord start, EditorCoord end)
{
  if (start == end)
  {
    return false;
  }

  bool has_intersection = false;

  // Iterate all lines and check if they do not intersect with this one
  this->for_each_object_of_type<EditorLine>([&](EditorLine& line) -> bool
  {
    EditorHalfEdge& half_edge_a = this->get_object<EditorHalfEdge>(line.half_edge_a);
    EditorHalfEdge& half_edge_b = this->get_object<EditorHalfEdge>(line.half_edge_b);
    EditorCoord     other_a     = this->get_object<EditorPoint>(half_edge_a.from).coords;
    EditorCoord     other_b     = this->get_object<EditorPoint>(half_edge_b.from).coords;

    has_intersection = segments_intersect(start, end, other_a, other_b);
    return !has_intersection; // Stop at the first one we hit
  });

  return !has_intersection;
}

//==================================================================================================
// Answers exactly the same question as is_sector_within_sector and picks the midpoints in exactly
// the same way, but decides each one differently. Instead of summing up the angles it shoots a ray
// to the left from the midpoint and counts how many edges of sector2 it crosses - an odd count
// means we are inside. This is the crossing number (also called even-odd, or ray casting) variant
// of the test and unlike the winding one it needs no floating point math at all.
bool is_sector_within_sector
(
  const EditorLevel& level, EID<EditorSector> sector1_id, EID<EditorSector> sector2_id
)
{
  const EditorSector& sector1 = level.get_object<EditorSector>(sector1_id);
  const EditorSector& sector2 = level.get_object<EditorSector>(sector2_id);

  EditorID half_edge_id = sector1.edge;
  do
  {
    const EditorHalfEdge& half_edge      = level.get_object<EditorHalfEdge>(half_edge_id);
    const EditorHalfEdge& half_edge_twin = level.get_object<EditorHalfEdge>(half_edge.twin);
    EditorCoord c1 = level.get_object<EditorPoint>(half_edge.from     ).coords;
    EditorCoord c2 = level.get_object<EditorPoint>(half_edge_twin.from).coords;

    // The same doubling trick as in the winding version - we keep the midpoint multiplied by 2 so
    // that we never lose the odd coordinates and multiply sector2 by 2 as well.
    EditorCoord doubled_midpoint = c1 + c2;

    // Count the edges of sector2 that our ray crosses on its way to the left
    s32  crossings = 0;
    bool on_edge   = false;

    EditorID outer_sector_edge_id = sector2.edge;
    do
    {
      const EditorHalfEdge& outer_edge      = level.get_object<EditorHalfEdge>(outer_sector_edge_id);
      const EditorHalfEdge& outer_edge_twin = level.get_object<EditorHalfEdge>(outer_edge.twin);
      EditorCoord outer_c1 = level.get_object<EditorPoint>(outer_edge.from     ).coords * 2;
      EditorCoord outer_c2 = level.get_object<EditorPoint>(outer_edge_twin.from).coords * 2;

      s64 edge_x = cast<s64>(outer_c2.x) - outer_c1.x;
      s64 edge_y = cast<s64>(outer_c2.y) - outer_c1.y;
      s64 to_m_x = cast<s64>(doubled_midpoint.x) - outer_c1.x;
      s64 to_m_y = cast<s64>(doubled_midpoint.y) - outer_c1.y;

      // Both halves of the determinant. Keeping them apart lets us answer both questions below
      // without ever doing a division.
      s64 det_left  = to_m_y * edge_x;
      s64 det_right = to_m_x * edge_y;

      // Equal halves mean we lie exactly on the line of the edge, so check the bounds as well. If
      // we are within them then this midpoint sits on the boundary and tells us nothing.
      if (det_left == det_right                                      &&
          std::min(outer_c1.x, outer_c2.x) <= doubled_midpoint.x     &&
          std::max(outer_c1.x, outer_c2.x) >= doubled_midpoint.x     &&
          std::min(outer_c1.y, outer_c2.y) <= doubled_midpoint.y     &&
          std::max(outer_c1.y, outer_c2.y) >= doubled_midpoint.y)
      {
        on_edge = true;
        break;
      }

      // Only edges that straddle our row can be crossed. The comparison is half open on purpose -
      // that way horizontal edges count for nothing and a vertex sitting exactly on our row counts
      // once when the boundary passes through it and not at all when it only touches and turns back.
      if ((outer_c1.y > doubled_midpoint.y) != (outer_c2.y > doubled_midpoint.y))
      {
        // Dividing would give us the X coordinate of the crossing, so instead we just compare both
        // sides of that division and flip it for the edges that go downwards.
        bool crosses_on_the_left = edge_y > 0 ? det_left < det_right : det_left > det_right;
        if (crosses_on_the_left)
        {
          ++crossings;
        }
      }

      outer_sector_edge_id = outer_edge.next;
    } while(outer_sector_edge_id != sector2.edge);

    if (!on_edge)
    {
      // This midpoint is strictly on one side or the other, so it decides for the whole sector
      return (crossings % 2) == 1;
    }

    half_edge_id = half_edge.next;
  } while(half_edge_id != sector1.edge);

  // Every edge of sector1 is shared with sector2, so the two only touch and do not contain
  // each other.
  return false;
}

//==================================================================================================
// Finds the deepest sector that contains the given one, starting the search in the given parent.
// The parent itself is returned if none of its holes contains the sector.
static EditorID find_parent_sector(const EditorLevel& level, EditorID parent_id, EditorID sector_id)
{
  const EditorSector& top = level.get_object<EditorSector>(parent_id);

  // Search through all sub-sectors
  EditorID hole_id = top.first_hole;

  if (hole_id != INVALID_EDITOR_ID)
  {
    do
    {
      if (hole_id != sector_id && is_sector_within_sector(level, sector_id, hole_id))
      {
        // Continue recursively down if possible
        return find_parent_sector(level, hole_id, sector_id);
      }

      // Not inside? Then try another sector..
      const auto& hole_sector = level.get_object<EditorSector>(hole_id);
      hole_id = hole_sector.next_hole;
    } while (hole_id != top.first_hole);
  }

  // None of the holes contains us, so we lie directly inside of the parent
  return parent_id;
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
// Area of the axis aligned bounding box around a loop of half edges. We use it to tell which of two
// sectors is the bigger one, both when splitting one into two and when merging two back together.
static s64 bbox_area_of_loop
(
  const EditorLevel& level, const StackVector<EID<EditorHalfEdge>, 32>& edges
)
{
  nc_assert(edges.size() > 0);

  EditorCoord min_coords = level.get_object<EditorPoint>(level.get_object<EditorHalfEdge>(edges.front()).from).coords;
  EditorCoord max_coords = min_coords;

  for (EID<EditorHalfEdge> edge_id : edges)
  {
    EditorCoord coords = level.get_object<EditorPoint>(level.get_object<EditorHalfEdge>(edge_id).from).coords;

    min_coords.x = std::min(min_coords.x, coords.x);
    min_coords.y = std::min(min_coords.y, coords.y);
    max_coords.x = std::max(max_coords.x, coords.x);
    max_coords.y = std::max(max_coords.y, coords.y);
  }

  return cast<s64>(max_coords.x - min_coords.x) * cast<s64>(max_coords.y - min_coords.y);
}

//==================================================================================================
// Chains the edges into a cycle and hands the whole loop over to the given sector. The sector then
// points to the lowest ID edge of the loop - any of them would do, but always picking the same one
// means the sector ends up identical no matter which edge the walk happened to start from.
static void assign_loop_to_sector
(
  EditorLevel& level, const StackVector<EID<EditorHalfEdge>, 32>& edges, EID<EditorSector> sector_id
)
{
  EID<EditorHalfEdge> lowest_edge_id = edges.front();

  for (u64 edge_id_idx = 0; edge_id_idx < edges.size(); ++edge_id_idx)
  {
    EID<EditorHalfEdge> edge_id      = edges[edge_id_idx];
    EID<EditorHalfEdge> next_edge_id = edges[(edge_id_idx + 1) % edges.size()];

    EditorHalfEdge& edge = level.get_object<EditorHalfEdge>(edge_id);
    edge.sector = sector_id;
    edge.next   = next_edge_id;

    lowest_edge_id = std::min(lowest_edge_id, edge_id);
  }

  level.get_object<EditorSector>(sector_id).edge = lowest_edge_id;
}

//==================================================================================================
// Inserts a hole into the sector's hole list. The list is kept sorted by the ID in an ascending
// order with the first hole always being the lowest ID one. That way the same set of holes always
// produces the same list, no matter in which order they were inserted.
static void insert_hole(EditorLevel& level, EditorSector& sector, EID<EditorSector> hole_id)
{
  if (sector.first_hole == INVALID_EDITOR_ID)
  {
    sector.first_hole = hole_id;
    level.get_object<EditorSector>(hole_id).next_hole = hole_id;
    return;
  }

  if (hole_id < sector.first_hole)
  {
    // We are the new lowest ID, so we belong right before the current first hole, which means right
    // after the last one.
    EID<EditorSector> last_id = sector.first_hole;
    while (level.get_object<EditorSector>(last_id).next_hole != sector.first_hole)
    {
      last_id = level.get_object<EditorSector>(last_id).next_hole;
    }

    level.get_object<EditorSector>(last_id).next_hole = hole_id;
    level.get_object<EditorSector>(hole_id).next_hole = sector.first_hole;
    sector.first_hole = hole_id;
    return;
  }

  // Find the last hole with a lower ID than ours and squeeze ourselves right after it
  EID<EditorSector> prev_id = sector.first_hole;
  while (true)
  {
    EID<EditorSector> next_id = level.get_object<EditorSector>(prev_id).next_hole;
    if (next_id == sector.first_hole || next_id > hole_id)
    {
      break;
    }

    prev_id = next_id;
  }

  level.get_object<EditorSector>(hole_id).next_hole = level.get_object<EditorSector>(prev_id).next_hole;
  level.get_object<EditorSector>(prev_id).next_hole = hole_id;
}

//==================================================================================================
// Unlinks a hole from the sector's hole list. Does nothing if it is not in there at all.
static void remove_hole(EditorLevel& level, EditorSector& sector, EID<EditorSector> hole_id)
{
  if (sector.first_hole == INVALID_EDITOR_ID)
  {
    return;
  }

  // Find the hole that points to the one we are removing. For a list with a single hole in it this
  // ends up being the hole itself, because it points back to itself.
  EID<EditorSector> prev_id = sector.first_hole;
  while (level.get_object<EditorSector>(prev_id).next_hole != hole_id)
  {
    prev_id = level.get_object<EditorSector>(prev_id).next_hole;
    if (prev_id == sector.first_hole)
    {
      // We went all the way around without finding it
      return;
    }
  }

  EditorSector& hole = level.get_object<EditorSector>(hole_id);

  if (hole.next_hole == hole_id)
  {
    // We were the only one in the list
    sector.first_hole = INVALID_EDITOR_ID;
  }
  else
  {
    level.get_object<EditorSector>(prev_id).next_hole = hole.next_hole;

    if (sector.first_hole == hole_id)
    {
      sector.first_hole = hole.next_hole;
    }
  }

  hole.next_hole = INVALID_EDITOR_ID;
}

//==================================================================================================
// Moves every hole of one sector into another one, leaving the source with no holes at all. Used
// when a sector stops existing and everything that was inside of it has to go somewhere else.
static void move_holes(EditorLevel& level, EID<EditorSector> from_id, EID<EditorSector> to_id)
{
  EditorSector& from = level.get_object<EditorSector>(from_id);
  if (from.first_hole == INVALID_EDITOR_ID)
  {
    return;
  }

  // Collect them first, we can not walk the list while we are relinking it
  StackVector<EID<EditorSector>, 16> holes;
  EID<EditorSector> hole_rover = from.first_hole;
  do
  {
    holes.push_back(hole_rover);
    hole_rover = level.get_object<EditorSector>(hole_rover).next_hole;
  } while (hole_rover != from.first_hole);

  from.first_hole = INVALID_EDITOR_ID;

  // And now insert them one by one so that the target list stays sorted
  EditorSector& to = level.get_object<EditorSector>(to_id);
  for (EID<EditorSector> hole_id : holes)
  {
    level.get_object<EditorSector>(hole_id).parent = to_id;
    insert_hole(level, to, hole_id);
  }
}

//==================================================================================================
bool EditorLevel::create_line(EditorID line_id, EditorCoord start, EditorCoord end)
{
  nc_assert(this->can_create_line(start, end));
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

  // Negative sign, reverse..
  if (is_zero(left_signed_angle + PI2, 0.1f))
  {
    left_enclosed_edges.clear();
    iterate_left_most_half_edges(*this, h2_id, left_signed_angle, left_enclosed_edges);
    std::swap(h1_id, h2_id); // Swap them
  }

  // Now that we returned back we should have a list of points through which we travelled
  // Check if the area is enclosed by examining the total signed angle
  if (!is_zero(left_signed_angle - PI2, 0.1f))
  {
    // This means that we haven't created any new sector or split another sector into 2..
    // Bail out.
    // The line was however created succesfully, so return success.
    return false;
  }

  // If the sum is ~360 degrees then the area forms a cycle! This means that we either created a
  // brand new sector or split one sector into 2.
  nc_assert(left_enclosed_edges.size() >= 3);

  auto connect_holes = [this](const StackVector<EditorID, 16>& holes)
  {
    for (u64 i_curr = 0; i_curr < holes.size(); ++i_curr)
    {
      u64 i_next = (i_curr+1) % holes.size();
      this->get_object<EditorSector>(holes[i_curr]).next_hole = holes[i_next];
    }
  };

  // Was this previously a sector that we split, or is it a new one? Any edge of the loop that
  // already belongs somewhere answers that. We can not just look at the first one, because the two
  // edges we have created belong nowhere yet and neither do the rims of the holes we might have
  // just bridged into, so the loop can easily start with a couple of those.
  EID<EditorSector> previous_sector_id = INVALID_EDITOR_ID;
  for (EID<EditorHalfEdge> edge_id : left_enclosed_edges)
  {
    EID<EditorSector> edge_sector_id = this->get_object<EditorHalfEdge>(edge_id).sector;
    if (edge_sector_id != INVALID_EDITOR_ID)
    {
      previous_sector_id = edge_sector_id;
      break;
    }
  }

  // This was previously an existing sector that we have split
  bool did_sector_splitting = previous_sector_id != INVALID_EDITOR_ID;
  bool connected_parent_to_hole = false;

  // Now the question is if we split it into 2 sectors, or just connected hole & parent and
  // therefore just changed the parent sector and no new sector is created.
  if (did_sector_splitting)
  {
    iterate_left_most_half_edges(*this, h2_id, right_signed_angle, right_enclosed_edges);

    // Now check if we visited h1_id
    for (EID<EditorHalfEdge> hedge : right_enclosed_edges)
    {
      if (hedge == h1_id)
      {
        // This means that we connected parent with a hole and haven't created a new sector, just
        // made everything more complex..
        connected_parent_to_hole = true;
        break;
      }
    }
  }

  // Now there are 3 cases..
  // a) Split one sector into 2 sectors, create a new sector.
  //  - Have to reparent all holes
  // b) Connected parent with a hole, therefore haven't created any new sectors
  //  - Still have to potentially reparent holes
  // c) Created a brand new sector from edges that were not a part of any sector before.
  bool created_new_sector = !connected_parent_to_hole;
  if (created_new_sector)
  {
    EID<EditorSector> new_sector_id = this->new_id();
    EditorSector&     new_sector    = this->create_object<EditorSector>(new_sector_id);

    if (did_sector_splitting) // (A)
    {
      nc_assert(is_zero(right_signed_angle - PI2, 0.1f)); // This must hold if it was a sector previously

      // The bigger of the two halves keeps the identity of the sector we have just split. Deleting
      // the line again merges them back and lets the bigger one survive as well, so the sector that
      // was here before the split is the one that comes out of it.
      bool left_keeps_the_old_sector =
        bbox_area_of_loop(*this, left_enclosed_edges) >= bbox_area_of_loop(*this, right_enclosed_edges);

      assign_loop_to_sector(*this, left_keeps_the_old_sector ? left_enclosed_edges  : right_enclosed_edges, previous_sector_id);
      assign_loop_to_sector(*this, left_keeps_the_old_sector ? right_enclosed_edges : left_enclosed_edges,  new_sector_id     );

      EditorSector& prev_sector = this->get_object<EditorSector>(previous_sector_id);

      // The parent has to be the same as for the previous sector obviously, because we just split
      // the old sector into 2 new ones.
      new_sector.parent     = prev_sector.parent;
      new_sector.first_hole = INVALID_EDITOR_ID; // Preventively set it here, might get changed later

      // We need to redistribute previous hole sectors into the 2 sectors - one new one and one old
      // one. We have to do this before we patch the hole list
      if (prev_sector.first_hole != INVALID_EDITOR_ID)
      {
        EID<EditorSector> hole_rover = prev_sector.first_hole;
        StackVector<EditorID, 16> new_sector_holes, old_sector_holes;

        // Iterate all old sector holes and decide if they should become holes of the old sector or
        // the new one. Collect them into 2 lists that we will later use to connect their pointers.
        do
        {
          EditorSector& hole = this->get_object<EditorSector>(hole_rover);
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

        // And now iterate both lists and connect them appropriately
        connect_holes(new_sector_holes);
        connect_holes(old_sector_holes);

        // And appoint the correct first hole to both new and old sectors
        prev_sector.first_hole = old_sector_holes.size() ? old_sector_holes.front() : INVALID_EDITOR_ID;
        new_sector.first_hole  = new_sector_holes.size() ? new_sector_holes.front() : INVALID_EDITOR_ID;
      }

      // Now we need to fix the hole-list. We became a sibling of the sector we have just split, so
      // we belong into the same parent hole list as it does.
      insert_hole(*this, this->get_object<EditorSector>(new_sector.parent), new_sector_id);
    }
    else // (C)
    {
      // This means that we created a brand new sector.
      assign_loop_to_sector(*this, left_enclosed_edges, new_sector_id);

      // Find correct parent
      new_sector.parent = find_parent_sector(*this, VOID_SECTOR_ID, new_sector_id);
      EditorSector& parent = this->get_object<EditorSector>(new_sector.parent);

      // The new sector might have been drawn around sectors that already exist. Those are holes of
      // our parent at the moment, but now they lie inside of us, so they become our holes instead.
      // Anything nested deeper comes along with them.
      if (parent.first_hole != INVALID_EDITOR_ID)
      {
        StackVector<EID<EditorSector>, 16> adopted_holes;

        EID<EditorSector> hole_rover = parent.first_hole;
        do
        {
          if (is_sector_within_sector(*this, hole_rover, new_sector_id))
          {
            adopted_holes.push_back(hole_rover);
          }

          hole_rover = this->get_object<EditorSector>(hole_rover).next_hole;
        } while (hole_rover != parent.first_hole);

        // Relink only after the walk, removing them from the list while walking it would break it
        for (EID<EditorSector> hole_id : adopted_holes)
        {
          remove_hole(*this, parent, hole_id);
          this->get_object<EditorSector>(hole_id).parent = new_sector_id;
          insert_hole(*this, new_sector, hole_id);
        }
      }

      // And put ourselves into the parent hole list
      insert_hole(*this, parent, new_sector_id);
    }
  }
  else // (B)
  {
    EditorSector& prev_sector = this->get_object<EditorSector>(previous_sector_id);

    // There might be some new edges, have to resolve them
    assign_loop_to_sector(*this, left_enclosed_edges, previous_sector_id);

    // The holes might not be holes anymore.. Have to check all of them and possibly reparent under
    // our parent.
    if (prev_sector.first_hole != INVALID_EDITOR_ID)
    {
      StackVector<EID<EditorSector>, 16> remaning_holes, reparent_holes;

      EID<EditorSector> hole_rover = prev_sector.first_hole;
      do
      {
        EditorSector& hole = this->get_object<EditorSector>(hole_rover);

        if (is_sector_within_sector(*this, hole_rover, previous_sector_id))
        {
          remaning_holes.push_back(hole_rover);
        }
        else
        {
          reparent_holes.push_back(hole_rover);
          hole.parent = prev_sector.parent; // Put into our parent
        }

        hole_rover = hole.next_hole;
      }
      while (hole_rover != prev_sector.first_hole);

      // Connect the remaining holes under us into a list and then assign the first one
      connect_holes(remaning_holes);
      prev_sector.first_hole = remaning_holes.size() ? remaning_holes.front() : INVALID_EDITOR_ID;

      if (reparent_holes.size())
      {
        EditorSector& parent = this->get_object<EditorSector>(prev_sector.parent);

        // The holes that went a level up have to be merged into the parent hole list one by one so
        // that it stays sorted.
        for (EID<EditorSector> hole_id : reparent_holes)
        {
          insert_hole(*this, parent, hole_id);
        }
      }
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
bool EditorLevel::destroy_line(EditorID line_id)
{
  EditorLine* line = this->try_get_object<EditorLine>(line_id);
  if (line == nullptr)
  {
    // There is no such line, so there is nothing to destroy
    return false;
  }

  EID<EditorHalfEdge> h1_id  = line->half_edge_a;
  EID<EditorHalfEdge> h2_id  = line->half_edge_b;
  EID<EditorSector>   s1_id  = this->get_object<EditorHalfEdge>(h1_id).sector;
  EID<EditorSector>   s2_id  = this->get_object<EditorHalfEdge>(h2_id).sector;
  EID<EditorPoint>    pt1_id = this->get_object<EditorHalfEdge>(h1_id).from;
  EID<EditorPoint>    pt2_id = this->get_object<EditorHalfEdge>(h2_id).from;

  // Walk the loops of both touched sectors while we still can - once the two half edges are gone
  // their loops are broken and there is no way to find the rest of the edges anymore. The two edges
  // we are about to delete are left out, everything else survives us.
  StackVector<EID<EditorHalfEdge>, 32> s1_edges, s2_edges;

  auto collect_loop = [&](EID<EditorSector> sector_id, StackVector<EID<EditorHalfEdge>, 32>& out)
  {
    if (sector_id == INVALID_EDITOR_ID)
    {
      return;
    }

    EID<EditorHalfEdge> first_edge_id = this->get_object<EditorSector>(sector_id).edge;
    EID<EditorHalfEdge> edge_rover    = first_edge_id;
    do
    {
      if (edge_rover != h1_id && edge_rover != h2_id)
      {
        out.push_back(edge_rover);
      }

      edge_rover = this->get_object<EditorHalfEdge>(edge_rover).next;
    } while (edge_rover != first_edge_id);
  };

  collect_loop(s1_id, s1_edges);
  if (s2_id != s1_id)
  {
    collect_loop(s2_id, s2_edges);
  }

  // Now tear out the line together with both of its half edges. This also unlinks them from the
  // point_to_half_edges index, which all the loop walking below relies on.
  this->destroy_object(line_id);
  this->destroy_object(h1_id);
  this->destroy_object(h2_id);

  // The endpoints might have been used by this line only, in which case they go away with it.
  // The point gets removed from the list if there are no half-edges associated with it during
  // the "destroy_object" call.
  if (!this->point_to_half_edges.contains(pt1_id))
  {
    this->destroy_object(pt1_id);
  }

  if (!this->point_to_half_edges.contains(pt2_id))
  {
    this->destroy_object(pt2_id);
  }

  // Neither side of the line was a sector, so there is nothing else to fix up
  if (s1_id == INVALID_EDITOR_ID && s2_id == INVALID_EDITOR_ID)
  {
    return true;
  }

  // Both sides belonged to the same sector. That means the line was either a bridge connecting the
  // sector with one of its holes, or just a spur sticking into it. Removing a bridge splits the
  // loop back into two, removing a spur only makes the loop shorter.
  if (s1_id == s2_id)
  {
    nc_assert(s1_edges.size() >= 3);

    f32                                  first_angle = 0.0f;
    StackVector<EID<EditorHalfEdge>, 32> first_loop;
    iterate_left_most_half_edges(*this, s1_edges.front(), first_angle, first_loop);

    // Did the first loop visit everything that is left? If not then the loop fell apart into two
    // and the second one is the rim of a hole that we have just disconnected.
    EID<EditorHalfEdge> unvisited_id = INVALID_EDITOR_ID;
    for (EID<EditorHalfEdge> edge_id : s1_edges)
    {
      if (std::find(first_loop.begin(), first_loop.end(), edge_id) == first_loop.end())
      {
        unvisited_id = edge_id;
        break;
      }
    }

    if (unvisited_id == INVALID_EDITOR_ID)
    {
      // It was only a spur, so the sector just lost a couple of edges and stays as it was
      assign_loop_to_sector(*this, first_loop, s1_id);
      return true;
    }

    f32                                  second_angle = 0.0f;
    StackVector<EID<EditorHalfEdge>, 32> second_loop;
    iterate_left_most_half_edges(*this, unvisited_id, second_angle, second_loop);

    // The loop that winds around counter clockwise encloses the sector, the other one goes the
    // other way around and is therefore the rim of the hole.
    bool first_is_the_boundary = is_zero(first_angle - PI2, 0.1f);
    auto& boundary_loop = first_is_the_boundary ? first_loop  : second_loop;
    auto& hole_rim_loop = first_is_the_boundary ? second_loop : first_loop;

    assign_loop_to_sector(*this, boundary_loop, s1_id);

    // The rim goes back to belonging to nobody, exactly like it did before the bridge was created.
    // The hole on its other side is what represents it from now on.
    for (EID<EditorHalfEdge> edge_id : hole_rim_loop)
    {
      EditorHalfEdge& edge = this->get_object<EditorHalfEdge>(edge_id);
      edge.sector = INVALID_EDITOR_ID;
      edge.next   = INVALID_EDITOR_ID;
    }

    // And whatever sector lies on the other side of the rim becomes our hole again
    EID<EditorHalfEdge> rim_twin_id = this->get_object<EditorHalfEdge>(hole_rim_loop.front()).twin;
    EID<EditorSector>  hole_id     = this->get_object<EditorHalfEdge>(rim_twin_id).sector;

    if (hole_id != INVALID_EDITOR_ID && hole_id != s1_id)
    {
      EditorSector& hole = this->get_object<EditorSector>(hole_id);

      remove_hole(*this, this->get_object<EditorSector>(hole.parent), hole_id);
      hole.parent = s1_id;
      insert_hole(*this, this->get_object<EditorSector>(s1_id), hole_id);
    }

    return true;
  }

  // Only one of the sides was a sector, which means the other one was either the void or the inside
  // of some enclosing sector. Either way the sector is open now and stops existing.
  if (s1_id == INVALID_EDITOR_ID || s2_id == INVALID_EDITOR_ID)
  {
    EID<EditorSector> dead_id   = s1_id != INVALID_EDITOR_ID ? s1_id : s2_id;
    EID<EditorSector> parent_id = this->get_object<EditorSector>(dead_id).parent;

    // Everything that used to be inside of it now lies directly inside of its parent
    move_holes(*this, dead_id, parent_id);
    remove_hole(*this, this->get_object<EditorSector>(parent_id), dead_id);

    // The edges are still here, they just do not enclose anything anymore
    for (EID<EditorHalfEdge> edge_id : (s1_id != INVALID_EDITOR_ID ? s1_edges : s2_edges))
    {
      EditorHalfEdge& edge = this->get_object<EditorHalfEdge>(edge_id);
      edge.sector = INVALID_EDITOR_ID;
      edge.next   = INVALID_EDITOR_ID;
    }

    this->destroy_object(dead_id);
    return true;
  }

  // Two different sectors were sharing the line, so they merge into a single one. The bigger one
  // survives, which is the same rule that decides which half keeps the old sector when a line
  // splits one in two. On a tie the older sector wins, and because the IDs come from a counter that
  // only ever goes up, that is simply the lower one of the two.
  s64  s1_area     = bbox_area_of_loop(*this, s1_edges);
  s64  s2_area     = bbox_area_of_loop(*this, s2_edges);
  bool s1_survives = s1_area != s2_area ? s1_area > s2_area : s1_id < s2_id;

  EID<EditorSector> survivor_id = s1_survives ? s1_id : s2_id;
  EID<EditorSector> dead_id     = s1_survives ? s2_id : s1_id;

  // Both loops became a single one, so walk it and hand the whole thing over to the survivor
  f32                                  merged_angle = 0.0f;
  StackVector<EID<EditorHalfEdge>, 32> merged_loop;
  iterate_left_most_half_edges(*this, s1_edges.front(), merged_angle, merged_loop);
  nc_assert(is_zero(merged_angle - PI2, 0.1f));

  assign_loop_to_sector(*this, merged_loop, survivor_id);

  // Everything that was inside of the sector that just died is now inside of the merged one
  move_holes(*this, dead_id, survivor_id);
  remove_hole(*this, this->get_object<EditorSector>(this->get_object<EditorSector>(dead_id).parent), dead_id);
  this->destroy_object(dead_id);

  return true;
}

//==================================================================================================
bool EditorLevel::can_move_points(const std::map<EID<EditorPoint>, EditorCoord>& new_point_coords)
{
  // Iterate all moved points and for each one check if none of the lines outgoing from this point
  // intersects any other line.
  // We need to consider only points that actually moved.
  auto coords_of = [&](EID<EditorPoint> point_id) -> EditorCoord
  {
    auto it = new_point_coords.find(point_id);
    return it != new_point_coords.end() ? it->second : this->get_object<EditorPoint>(point_id).coords;
  };

  auto did_move = [&](EID<EditorPoint> point_id) -> bool
  {
    return coords_of(point_id) != this->get_object<EditorPoint>(point_id).coords;
  };

  // No point can end up on top of another one, that would turn two points into one and the lines
  // between them into nonsense. A point that is standing there is a problem, unless it moves away.
  std::unordered_set<EditorCoord> target_coords;
  for (const auto&[point_id, new_coord] : new_point_coords)
  {
    if (!did_move(point_id))
    {
      continue;
    }

    auto occupant = this->coord_to_point.find(new_coord);
    if (occupant != this->coord_to_point.end() && occupant->second != point_id && !did_move(occupant->second))
    {
      return false;
    }

    // Two moved points can not land on the same spot either
    if (!target_coords.insert(new_coord).second)
    {
      return false;
    }
  }

  // Collect all lines with their coords after the move. Only the ones that touch a moved point can
  // start intersecting something, the rest stayed where they were and did not intersect before.
  struct Segment
  {
    EditorCoord a, b;
    bool        moved;
  };

  std::vector<Segment> segments;
  this->for_each_object_of_type<EditorLine>([&](const EditorLine& line)
  {
    EID<EditorPoint> pt_a = this->get_object<EditorHalfEdge>(line.half_edge_a).from;
    EID<EditorPoint> pt_b = this->get_object<EditorHalfEdge>(line.half_edge_b).from;
    segments.push_back(Segment{coords_of(pt_a), coords_of(pt_b), did_move(pt_a) || did_move(pt_b)});
  });

  // Since no two points share a coordinate, sharing a coordinate means sharing the point and that
  // is exactly what segments_intersect forgives.
  for (u64 i = 0; i < segments.size(); ++i)
  {
    if (!segments[i].moved)
    {
      continue;
    }

    for (u64 j = 0; j < segments.size(); ++j)
    {
      // Pairs of two moved lines get checked only once
      if (i == j || (segments[j].moved && j < i))
      {
        continue;
      }

      if (segments_intersect(segments[i].a, segments[i].b, segments[j].a, segments[j].b))
      {
        return false;
      }
    }
  }

  return true;
}

//==================================================================================================
void EditorLevel::move_points(const std::map<EID<EditorPoint>, EditorCoord>& new_point_coords)
{
  nc_assert(this->can_move_points(new_point_coords));

  // Iterate the points and move them
  for (const auto&[point_id, new_coord] : new_point_coords)
  {
    this->get_object<EditorPoint>(point_id).coords = new_coord;
  }
}

//==================================================================================================
void dump_sector_and_subsectors(const EditorLevel& level, EditorID sector_id, s32 indent)
{
  const EditorSector& sector = level.get_object<EditorSector>(sector_id);

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
      rover_hole = level.get_object<EditorSector>(rover_hole).next_hole;
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
  dump_sector_and_subsectors(*this, VOID_SECTOR_ID, 0);
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
// Walks every case that destroy_line has to handle, dumping the level after each step so that the
// merges and the reappearing holes can be eyeballed.
void test_editor_level_destroy()
{
  EditorLevel level;

  ivec2 sw = ivec2{-10, -10};
  ivec2 se = ivec2{ 10, -10};
  ivec2 ne = ivec2{ 10,  10};
  ivec2 nw = ivec2{-10,  10};

  // A big square, split down the middle into two halves
  ivec2 s_mid = ivec2{0, -10};
  ivec2 n_mid = ivec2{0,  10};

  level.create_line(level.new_id(), sw,    s_mid);
  level.create_line(level.new_id(), s_mid, se   );
  level.create_line(level.new_id(), se,    ne   );
  level.create_line(level.new_id(), ne,    n_mid);
  level.create_line(level.new_id(), n_mid, nw   );
  level.create_line(level.new_id(), nw,    sw   );

  EditorID divider_id = level.new_id();
  level.create_line(divider_id, s_mid, n_mid);

  // std::cout << "Two halves of a square:" << std::endl;
  level.dump_to_text();

  // A small square nested inside of the left half, so that the merge below has a hole to carry over
  ivec2 hole_sw = ivec2{-7, -3};
  ivec2 hole_se = ivec2{-3, -3};
  ivec2 hole_ne = ivec2{-3,  3};
  ivec2 hole_nw = ivec2{-7,  3};

  level.create_line(level.new_id(), hole_sw, hole_se);
  level.create_line(level.new_id(), hole_se, hole_ne);
  level.create_line(level.new_id(), hole_ne, hole_nw);
  level.create_line(level.new_id(), hole_nw, hole_sw);

  // std::cout << "With a hole in the left half:" << std::endl;
  level.dump_to_text();

  // Bridge the left half with its hole, which should push the hole a level up
  EditorID bridge_id = level.new_id();
  level.create_line(bridge_id, nw, hole_nw);

  // std::cout << "Left half bridged to its hole:" << std::endl;
  level.dump_to_text();

  // Deleting the bridge has to hand the hole back to the left half
  level.destroy_line(bridge_id);
  // std::cout << "Bridge deleted, the hole should be back:" << std::endl;
  level.dump_to_text();

  // Deleting the divider merges both halves into one, which has to adopt the hole
  level.destroy_line(divider_id);
  // std::cout << "Divider deleted, the two halves should be merged:" << std::endl;
  level.dump_to_text();

  // Opening the outer square up to the void kills it off and hands the hole over to the void
  EditorID roof_id = INVALID_EDITOR_ID;
  level.for_each_object_of_type<EditorLine>([&](EditorID id, const EditorLine& line)
  {
    EditorCoord a = level.get_object<EditorPoint>(level.get_object<EditorHalfEdge>(line.half_edge_a).from).coords;
    EditorCoord b = level.get_object<EditorPoint>(level.get_object<EditorHalfEdge>(line.half_edge_b).from).coords;

    if ((a == ne && b == n_mid) || (a == n_mid && b == ne))
    {
      roof_id = id;
    }
  });

  nc_assert(roof_id != INVALID_EDITOR_ID);
  level.destroy_line(roof_id);
  // std::cout << "Outer square opened up, only the small square should be left:" << std::endl;
  level.dump_to_text();

  // And the leftover lines belong to nobody, so deleting them touches no sector at all
  level.destroy_line(roof_id); // Already gone, has to be a no-op
  // std::cout << "Deleting the same line again changes nothing:" << std::endl;
  level.dump_to_text();

  exit(68);
}

}

#endif // #if NC_EDITOR
