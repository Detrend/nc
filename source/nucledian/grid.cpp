// Project Nucledian Source File
#include <grid.h>
#include <common.h>

#include <math/vector.h>
#include <math/lingebra.h>
#include <intersect.h>

#include <algorithm>    // std::clamp

namespace nc::grid_helper
{

//==============================================================================
static auto remap_point_to_indices(
  vec2 point,
  vec2 min,
  vec2 max,
  u64  width,
  u64  height)
{
  nc_assert(min.x < max.x && min.y < max.y);

  // clamp the point to [min, max]
  point = nc::min(nc::max(point, min), max);

  // remap to interval [0, 1]
  const auto map_size = max-min;
  const auto remapped = (point-min) / map_size;

  const auto grid = vec2{static_cast<f32>(width), static_cast<f32>(height)};
  const auto remapped_to_grid = remapped * grid;

  struct Output
  {
    u64 xindex;
    u64 yindex;
  };

  const u64 x = static_cast<u64>(remapped_to_grid.x);
  const u64 y = static_cast<u64>(remapped_to_grid.y);

  return Output
  {
    .xindex = std::clamp<u64>(x, 0, width-1),
    .yindex = std::clamp<u64>(y, 0, height-1),
  };
}

}

namespace nc
{

//==============================================================================
template<typename T>
void StatGridAABB2<T>::initialize(u64 width, u64 height, vec2 min, vec2 max)
{
  nc_assert(m_initialized == false);
  nc_assert(width > 0 && height > 0);
  nc_assert(min.x < max.x && min.y < max.y);

  m_min = min;
  m_max = max;

  m_cells.resize(width);

  for (auto& cell : m_cells)
  {
    cell.resize(height);
  }

  m_initialized = true;
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_point(vec2 point, Visitor func) const
{
  this->query_aabb(aabb2{point}, func);
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_aabb(aabb2 bbox, Visitor func) const
{
  nc_assert(m_initialized);

  auto[xfrom, yfrom] = grid_helper::remap_point_to_indices(
    bbox.min, m_min, m_max, m_cells.size(), m_cells[0].size());

  auto[xto, yto] = grid_helper::remap_point_to_indices(
    bbox.max, m_min, m_max, m_cells.size(), m_cells[0].size());

  for (auto x = xfrom; x <= xto; ++x)
  {
    for (auto y = yfrom; y <= yto; ++y)
    {
      nc_assert(x < m_cells.size());
      nc_assert(y < m_cells[x].size());

      for (auto&[other_bbox, val] : m_cells[x][y])
      {
        if (intersect::aabb_aabb_2d(bbox, other_bbox))
        {
          if (func(bbox, val))
          {
            return;
          }
        }
      }
    }
  }
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_ray
(
  vec3 from, vec3 to, f32 expand, Visitor func
) const
{
  return this->query_ray(from.xz(), to.xz(), expand, func);
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_ray
(
  vec2 from, vec2 to, f32 expand, Visitor func
) const
{
  auto bbox = aabb2{from, to};
  bbox.insert_point(bbox.min - vec2{expand});
  bbox.insert_point(bbox.max + vec2{expand});

  this->query_aabb(bbox, func);
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::insert(aabb2 new_bbox, const T& data)
{
  nc_assert(m_initialized);
  nc_assert(new_bbox.is_valid());

  auto[xfrom, yfrom] = grid_helper::remap_point_to_indices(
    new_bbox.min, m_min, m_max, m_cells.size(), m_cells[0].size());

  auto[xto, yto] = grid_helper::remap_point_to_indices(
    new_bbox.max, m_min, m_max, m_cells.size(), m_cells[0].size());

  nc_assert(xfrom <= xto && yfrom <= yto);

  for (u64 xi = xfrom; xi <= xto; ++xi)
  {
    nc_assert(xi < m_cells.size());
    Coll& col = m_cells[xi];

    for (u64 yi = yfrom; yi <= yto; ++yi)
    {
      nc_assert(yi < col.size());
      Cell& cell = col[yi];
      cell.push_back(Unit
      {
        .bbox = new_bbox,
        .data = data,
      });
    }
  }
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::reset()
{
  m_cells.clear();
  m_min = vec2{FLT_MAX};
  m_max = vec2{-FLT_MAX};
  m_initialized = false;
}

//==============================================================================
template class StatGridAABB2<u8>;
template class StatGridAABB2<u16>;
template class StatGridAABB2<u32>;
template class StatGridAABB2<u64>;
template class StatGridAABB2<void*>;

}

