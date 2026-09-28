// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <types.h>
#include <common.h>
#include <engine/editor/editor_types.h>
#include <math/vector.h>

#include <variant>
#include <optional>
#include <vector>
#include <map>
#include <unordered_map>
#include <bit>
#include <utility>
#include <tuple>

namespace nc
{

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

}

#endif // #if NC_EDITOR
