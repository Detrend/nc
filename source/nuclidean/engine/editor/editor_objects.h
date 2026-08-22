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

struct EditorLevel;

template<typename T>
using Opt = std::optional<T>;

struct EditorPoint
{
  EditorCoord coords;
};

struct EditorHalfEdge
{
  EditorID from   = INVALID_EDITOR_ID;
  EditorID twin   = INVALID_EDITOR_ID;
  EditorID next   = INVALID_EDITOR_ID;
  EditorID sector = INVALID_EDITOR_ID;
};

struct EditorLineData
{
  u16 something;
};

struct EditorLine
{
  EditorID       half_edge_a = INVALID_EDITOR_ID;
  EditorID       half_edge_b = INVALID_EDITOR_ID;
  EditorLineData data;
};

struct EditorSectorData
{

};

// TODO: Rename later
struct EditorSector2
{
  EditorID edge       = INVALID_EDITOR_ID;
  EditorID parent     = INVALID_EDITOR_ID;
  EditorID first_hole = INVALID_EDITOR_ID;
  EditorID next_hole  = INVALID_EDITOR_ID;
};

struct EditorEntity
{

};

using EditorObject = std::variant<EditorPoint, EditorLine, EditorHalfEdge, EditorSector2, EditorEntity>;


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
  void do_destroy(EditorLevel& level);
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

  EditorSector2 void_sector
  {
    .first_hole = INVALID_EDITOR_ID,
  };

  // Returns pointer to the object with the given ID. Nullptr if the object does not exist.
  // Asserts if the object exists and is of a different type.
  template<typename T>
  T* try_get_object(EditorID id)
  {
    auto it = objects.find(id);
    if (it == objects.end())
    {
      return nullptr;
    }

    T* typed = std::get_if<T>(&it->second);
    nc_assert(typed != nullptr);
    return typed;
  }

  template<typename T>
  T& get_object(EditorID id)
  {
    T* object = this->try_get_object<T>(id);
    nc_assert(object);
    return *object;
  }

  template<typename T>
  const T& get_object(EditorID id) const
  {
    return const_cast<EditorLevel*>(this)->get_object<T>(id);
  }

  EditorObject* get_any_object(EditorID id);

  bool get_point_on_coord(EditorCoord coord, EditorID& id_out);

  EditorID new_id() const;

  template<typename T, typename F>
  void for_each_object_of_type(F&& lambda)
  {
    for (auto&[id, obj] : objects)
    {
      if (T* typed = std::get_if<T>(&obj))
      {
        if constexpr (requires (bool cont){cont = lambda(id, *typed);})
        {
          if (!lambda(id, *typed))
          {
            return;
          }
        }
        else if constexpr (requires {lambda(id, *typed);})
        {
          lambda(id, *typed);
        }
        else if constexpr (requires (bool cont){cont = lambda(*typed);})
        {
          if (!lambda(*typed))
          {
            return;
          }
        }
        else
        {
          lambda(*typed);
        }
      }
    }
  }

  template<typename T, typename...Args>
  T& create_object(EditorID id, Args&&...arguments)
  {
    nc_assert(!objects.contains(id));

    auto[it, ok] = objects.emplace
    (
      std::piecewise_construct, std::forward_as_tuple(id),
      std::forward_as_tuple(std::in_place_type<T>, std::forward<Args>(arguments)...)
    );

    nc_assert(ok);
    T& ref = std::get<T>(it->second);

    if constexpr (requires {this->on_object_created(id, ref);})
    {
      this->on_object_created(id, ref);
    }

    return ref;
  }

  void destroy_object(EditorID id);

  bool can_create_line(EditorCoord start, EditorCoord end);

  bool create_line(EditorID line_id, EditorCoord start, EditorCoord end);

  void destroy_line(EditorID edge_id);

  // Callback helpers
  void on_object_created(EditorID   id, const EditorPoint&    point    );
  void on_object_destroyed(EditorID id, const EditorPoint&    point    );
  void on_object_created(EditorID   id, const EditorHalfEdge& half_edge);

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
