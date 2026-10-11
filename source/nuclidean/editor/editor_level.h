// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>
#include <editor/editor_objects.h>

#include <map>
#include <unordered_map>
#include <vector>
#include <functional>


// The current plan:
// We want to support the following editor operations:
// - Create Line
//   . Creates a line between two points
//   . If this results in encircling of a sector then a sector is created
//   . This can also split one sector into two
//   . Or reorder holes
// - Destroy Line
//   . Removes a line
//   . This can remove a sector
// - Move Sector/Line/Points/Entity (s)
//   . Moves a set of objects
//   . This can have the same consequences as creating a line or destroying it
// - Splice Edge
//   . Adds additional point onto an edge, transforming it into 2 edges
// - Contract Point
//   . Removes a point and merges the 2 edges that were connected to it

namespace nc
{

struct EditorLevel
{
  using ObjectMap           = std::map<EditorID, EditorObject>;
  using PointToHalfEdgesMap = std::unordered_map<EID<EditorPoint>, std::vector<EID<EditorHalfEdge>>>;
  using CoordToPointMap     = std::unordered_map<EditorCoord, EID<EditorPoint>>;

  using SplitSectorCallback   = std::function<void(EditorID original_id, const EditorSectorData& original_data, EditorID& left_id, EditorSectorData& left_data, EditorID& right_id, EditorSectorData& right_data)>;
  using CreatedSectorCallback = std::function<void(EditorID& id_out, EditorSectorData& data_out)>;

  using MergedSectorsCallback   = std::function<void(EditorID left_id, const EditorSectorData& left_data, EditorID right_id, const EditorSectorData& right_data, EditorID& new_id, EditorSectorData& new_data)>;
  using DestroyedSectorCallback = std::function<void(EditorID sector_id, const EditorSectorData& sector_data)>;

  // Data themselves
  ObjectMap           objects;
  PointToHalfEdgesMap point_to_half_edges;
  CoordToPointMap     coord_to_point;
  mutable EditorID    next_free_id = 2;

  // Creates an empty level, which contains only the void sector
  EditorLevel();

  // Returns pointer to the object with the given ID. Nullptr if the object does not exist.
  // Asserts if the object exists and is of a different type.
  template<typename T>
  T* try_get_object(EditorID id);

  template<typename T>
  T& get_object(EditorID id);

  template<typename T>
  const T& get_object(EditorID id) const;

  EditorObject*       get_any_object(EditorID id);
  const EditorObject* get_any_object(EditorID id) const;

  bool get_point_on_coord(EditorCoord coord, EditorID& id_out);

  EditorID new_id() const;

  template<typename T, typename F>
  void for_each_object_of_type(F&& lambda);

  template<typename T, typename...Args>
  T& create_object(EditorID id, Args&&...arguments);

  void destroy_object(EditorID id);

  bool can_create_line(EditorCoord start, EditorCoord end);

  bool create_line
  (
    EditorID line_id, EditorCoord start, EditorCoord end,
    CreatedSectorCallback callback_created = nullptr, SplitSectorCallback callback_split = nullptr
  );

  bool destroy_line
  (
    EditorID line_id, DestroyedSectorCallback callback_destroyed = nullptr, MergedSectorsCallback callback_merged = nullptr
  );

  bool can_move_points(const std::map<EID<EditorPoint>, EditorCoord>& new_point_coords);

  EditorCoord get_line_coord(EditorID line_id, bool first) const;

  // Callback helpers
  void on_object_created(EditorID   id, const EditorPoint&    point    );
  void on_object_destroyed(EditorID id, const EditorPoint&    point    );
  void on_object_created(EditorID   id, const EditorHalfEdge& half_edge);
  void on_object_destroyed(EditorID id, const EditorHalfEdge& half_edge);

  // Debug function for printing the current status of the level into a text.
  void dump_to_text();
};

}

#include <editor/editor_level.inl>

#endif // #if NC_EDITOR
