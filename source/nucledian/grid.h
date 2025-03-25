// Project Nucledian Source File
#pragma once

#include <types.h>
#include <aabb.h>

#include <functional>
#include <utility>
#include <float.h>

namespace nc
{

// Static 2D grid storing bounding boxes.
// TODO: not very efficient, should use better data structure
template<typename T>
class StatGridAABB2
{
public:
  // Maybe RAII instead of this?
  void initialize(u64 width, u64 height, vec2 min, vec2 max);

  // Inserts a new AABB with some data into the grid.
  void insert(aabb2 bbox, const T& data);

  // Visitor returns true it it does not require more iterations
  using Visitor = std::function<bool(aabb2, const T&)>;
  void query_point(vec2 point, Visitor func) const;
  void query_aabb(aabb2 bbox,  Visitor func) const;

  // Resets the grid into non initialized state and frees all resources
  void reset();

private:
  struct Unit
  {
    aabb2 bbox; // bbox of the object
    T     data; // this will be some pointer, index or something else..
  };
  using Cell  = std::vector<Unit>;
  using Coll  = std::vector<Cell>;
  using Cells = std::vector<Coll>;

private:
  // MR says: I am just not a fan of std::numeric_limits<blah blah>...
  vec2  m_min = vec2{ FLT_MAX};
  vec2  m_max = vec2{-FLT_MAX};
  Cells m_cells;
  bool  m_initialized = false;
};

// TODO:  hash grid
// TODO2: hierarchical spatial hash grid
  
}

