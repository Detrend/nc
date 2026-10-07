// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_types.h>
#include <editor/editor_objects.h>

#include <map>
#include <unordered_map>
#include <vector>

namespace nc
{

struct EditorLevel
{
  using ObjectMap           = std::map<EditorID, EditorObject>;
  using PointToHalfEdgesMap = std::unordered_map<EID<EditorPoint>, std::vector<EID<EditorHalfEdge>>>;
  using CoordToPointMap     = std::unordered_map<EditorCoord, EID<EditorPoint>>;

  // Data themselves
  ObjectMap           objects;
  PointToHalfEdgesMap point_to_half_edges;
  CoordToPointMap     coord_to_point;

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

  bool create_line(EditorID line_id, EditorCoord start, EditorCoord end);

  bool destroy_line(EditorID line_id);

  bool can_move_points(const std::map<EID<EditorPoint>, EditorCoord>& new_point_coords);

  void move_points(const std::map<EID<EditorPoint>, EditorCoord>& new_point_coords);

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
