// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <common.h>
#include <math/vector.h>

#include <variant>
#include <optional>
#include <vector>
#include <map>
#include <unordered_map>
#include <bit>
#include <utility>
#include <tuple>

template<>
struct std::hash<nc::ivec2>
{
  std::size_t operator()(const nc::ivec2& vec) const noexcept
  {
    struct Shit
    {
      nc::u32 bottom;
      nc::u32 top;
    };

    Shit s;
    s.bottom = cast<nc::u32>(vec.x);
    s.top    = cast<nc::u32>(vec.y);

    return std::bit_cast<std::size_t>(s);
  }
};

namespace nc
{

using EditorID    = u64;
using EditorCoord = ivec2;

template<typename T>
using EID = EditorID;

constexpr EditorID INVALID_EDITOR_ID = 0;
constexpr EditorID VOID_SECTOR_ID    = 1;

struct EditorLevel;

struct EditorPoint
{
  EditorCoord coords;
  bool operator==(const EditorPoint&) const = default;
};

struct EditorHalfEdge
{
  EditorID from   = INVALID_EDITOR_ID;
  EditorID twin   = INVALID_EDITOR_ID;
  EditorID next   = INVALID_EDITOR_ID;
  EditorID sector = INVALID_EDITOR_ID;
  bool operator==(const EditorHalfEdge&) const = default;
};

struct EditorLineData
{
  u16 something;
  bool operator==(const EditorLineData&) const = default;
};

struct EditorLine
{
  EditorID       half_edge_a = INVALID_EDITOR_ID;
  EditorID       half_edge_b = INVALID_EDITOR_ID;
  EditorLineData data;
  bool operator==(const EditorLine&) const = default;
};

struct EditorSectorData
{
  bool operator==(const EditorSectorData&) const = default;
};

struct EditorSector
{
  EditorID edge       = INVALID_EDITOR_ID;
  EditorID parent     = INVALID_EDITOR_ID;
  EditorID first_hole = INVALID_EDITOR_ID;
  EditorID next_hole  = INVALID_EDITOR_ID;

  // void sector is a virtual non-existing sector that acts as a top-most parent of all other
  // sectors. It is tracked as a real sector to elliminate several edge-cases in the code even
  // though it does not have any edges.
  // Each top-most sector is tracked as a hole of the void sector.
  bool is_void() const
  {
    return parent == INVALID_EDITOR_ID; // Void is the only one that can have an invalid parent
  }

  bool operator==(const EditorSector&) const = default;
};

struct EditorEntity
{
  bool operator==(const EditorEntity&) const = default;
};

using EditorObject = std::variant<EditorPoint, EditorLine, EditorHalfEdge, EditorSector, EditorEntity>;

struct ActionCreateOrDeleteLine;
using EditorAction = std::variant<ActionCreateOrDeleteLine>;

struct ActionCreateOrDeleteLine
{
  bool        create        = true;
  bool        was_performed = false;
  EditorID    line_id;
  EditorCoord from;
  EditorCoord to;

  bool do_create(EditorLevel& level);
  bool do_destroy(EditorLevel& level);
  void action_do(EditorLevel& level);
  void action_undo(EditorLevel& level);
};

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

  EditorObject* get_any_object(EditorID id);

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

  // Callback helpers
  void on_object_created(EditorID   id, const EditorPoint&    point    );
  void on_object_destroyed(EditorID id, const EditorPoint&    point    );
  void on_object_created(EditorID   id, const EditorHalfEdge& half_edge);
  void on_object_destroyed(EditorID id, const EditorHalfEdge& half_edge);

  // Debug function for printing the current status of the level into a text.
  void dump_to_text();
};

template<typename ActionType>
void perform_action(EditorLevel& level, ActionType& action)
{
  action.action_do(level);
}

template<typename ActionType>
void undo_action(EditorLevel& level, ActionType& action)
{
  action.action_undo(level);
}

}

#include <engine/editor/editor_objects.inl>
