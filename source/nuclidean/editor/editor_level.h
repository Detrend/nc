// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>
#include <editor/editor_objects.h>

#include <stack_vector.h>

#include <map>
#include <unordered_map>
#include <vector>
#include <functional>

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

  // Called when a line needs a point on a coordinate where there is none yet. The callback can
  // leave the ID invalid, in which case a brand new one gets generated.
  using CreatedPointCallback = std::function<void(EditorCoord coord, EditorID& id_out)>;

  // Data themselves
  ObjectMap           objects;
  PointToHalfEdgesMap point_to_half_edges;
  CoordToPointMap     coord_to_point;
  mutable EditorID    next_free_id = 2; // MR says: this is a hack, rework later

  // Creates an empty level, which contains only the void sector
  EditorLevel();

  // Returns pointer to the object with the given ID. Nullptr if the object does not exist.
  // Asserts if the object exists and is of a different type.
  template<typename T>
  T* try_get_object(EditorID id);

  template<typename T>
  const T* try_get_object(EditorID id) const;

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

  // Returns an ID of a line between these 2 points or an invalid id if there is no such line
  EditorID get_line_between_points(EditorCoord start, EditorCoord end) const;

  // The half edge IDs can be left invalid, in which case they get generated.
  bool create_line
  (
    EditorID line_id, EditorCoord start, EditorCoord end,
    CreatedSectorCallback callback_created = nullptr, SplitSectorCallback callback_split = nullptr,
    CreatedPointCallback  callback_point   = nullptr,
    EditorID half_edge_a_id = INVALID_EDITOR_ID, EditorID half_edge_b_id = INVALID_EDITOR_ID
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

void iterate_left_most_half_edges
(
  const EditorLevel&                    level,
  EID<EditorHalfEdge>                   start_edge_id,
  f32&                                  total_signed_angle_out,
  StackVector<EID<EditorHalfEdge>, 32>& enclosed_edges_out
);

}

#include <editor/editor_level.inl>

#endif // #if NC_EDITOR
