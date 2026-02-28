// Project Nucledian Source File
#include <grid.h>
#include <common.h>

#include <math/vector.h>
#include <math/lingebra.h>
#include <intersect.h>
#include <profiling.h>

#include <algorithm>    // std::clamp
#include <set>          // std::set
#include <string>       // string for formatting

#ifdef NC_TESTS
#include <unit_test.h>  // NC_UNIT_TEST
#include <logging.h>    // nc_warn in tests
#endif

namespace nc::grid_helper
{

//==============================================================================
static auto remap_point_to_indices
(
  vec2 point,
  vec2 min,
  vec2 max,
  u64  width,
  u64  height
)
{
  nc_assert(min.x < max.x && min.y < max.y);

  // The following code is ok, but results in a non-inlining call. Too bad
  // f32 mxx = std::nextafter(max.x, -INFINITY);
  // f32 mxy = std::nextafter(max.y, -INFINITY);

  // Instead, lets use this variation and let's hope it does not fail
  f32 mxx = max.x - 0.001f;
  f32 mxy = max.y - 0.001f;
  // This might fire if the world size is too big
  nc_assert(mxx != max.x && mxy != max.y); 

  point = clamp(point, min, vec2{mxx, mxy});

  // Remap to interval [0, 1]
  vec2 map_size = max-min;
  vec2 remapped = (point-min) / map_size;
  
  vec2 grid = vec2{cast<f32>(width), cast<f32>(height)};
  vec2 remapped_to_grid = remapped * grid;

  struct Output
  {
    u64 xindex;
    u64 yindex;
  };

  return Output
  {
    .xindex = cast<u64>(remapped_to_grid.x),
    .yindex = cast<u64>(remapped_to_grid.y),
  };
}

//==============================================================================
static auto remap_point_to_indices_unclamped
(
  vec2 point,
  vec2 min,
  vec2 max,
  u64  width,
  u64  height
)
{
  nc_assert(min.x < max.x && min.y < max.y);

  // Remap to interval [0, 1]
  vec2 map_size = max-min;
  vec2 remapped = (point-min) / map_size;
  
  vec2  grid = vec2{cast<f32>(width), cast<f32>(height)};
  ivec2 remapped_to_grid = remapped * grid;

  nc_assert(remapped_to_grid.x >= 0);
  nc_assert(remapped_to_grid.y >= 0);
  nc_assert(cast<u64>(remapped_to_grid.x) < width);
  nc_assert(cast<u64>(remapped_to_grid.y) < height);

  struct Output
  {
    u64 xindex;
    u64 yindex;
  };

  return Output
  {
    .xindex = cast<u64>(remapped_to_grid.x),
    .yindex = cast<u64>(remapped_to_grid.y),
  };
}

//==============================================================================
// Returns false if the line is outside the bbox. Clips it otherwise and returns
// true.
// After this no point on the line lies outside the bbox.
static bool check_line_in_bbox_and_clip_it(vec2& from, vec2& to, aabb2 bbox)
{
  nc_assert(bbox.is_valid());

  vec2 mn = bbox.min;
  vec2 mx = bbox.max;

  // Going up/down
  if (from.x == to.x)
  {
    if (to.x != clamp(to.x, mn.x, mx.x))
    {
      return false;
    }

    if (max(from.y, to.y) < mn.y || min(from.y, to.y) > mx.y)
    {
      return false;
    }

    from.y = clamp(from.y, mn.y, mx.y);
    to.y   = clamp(to.y,   mn.y, mx.y);
    return true;
  }

  // Going left/right
  if (from.y == to.y)
  {
    if (to.y != clamp(to.y, mn.y, mx.y))
    {
      return false;
    }

    if (max(from.x, to.x) < mn.x || min(from.x, to.x) > mx.x)
    {
      return false;
    }

    from.x = clamp(from.x, mn.x, mx.x);
    to.x   = clamp(to.x,   mn.x, mx.x);
    return true;
  }

  // Now find 2 intersections
  vec2 dir = to - from;

  f32 cx1 = (mn.x - from.x) / dir.x;
  f32 cx2 = (mx.x - from.x) / dir.x;
  f32 cy1 = (mn.y - from.y) / dir.y;
  f32 cy2 = (mx.y - from.y) / dir.y;

  vec2 px1 = from + dir * cx1;
  vec2 px2 = from + dir * cx2;
  vec2 py1 = from + dir * cy1;
  vec2 py2 = from + dir * cy2;

  u8  cnt = 0;
  f32 cfs[4]{0, 0, 0, 0};

  if (px1.y >= mn.y && px1.y <= mx.y)
  {
    cfs[cnt++] = cx1;
  }

  if (px2.y >= mn.y && px2.y <= mx.y)
  {
    cfs[cnt++] = cx2;
  }

  if (py1.x >= mn.x && py1.x <= mx.x)
  {
    cfs[cnt++] = cy1;
  }

  if (py2.x >= mn.x && py2.x <= mx.x)
  {
    cfs[cnt++] = cy2;
  }

  // Happens if no intersection is valid
  if (cnt < 2)
  {
    return false;
  }

  f32 mni = max(min(cfs[0], cfs[1]), 0.0f);
  f32 mxi = min(max(cfs[0], cfs[1]), 1.0f);

  to   = from + dir * mxi;
  from = from + dir * mni;
  if (mni <= mxi)
  {
    // nc_assert(to == clamp(to, mn, mx) && from == clamp(from, mn, mx));
    // The following params trigger the assert :(
    // From:  {1.10000610   1.60455322 }
    // To:    {-533.628052 -523.461121 }
    // BBMin: {-67.4000015 -20.3999996 }
    // BBMax: {1052.59985   891.799866 }
    //
    // This seems to be just a float inaccuracy.
    // Solving this by clamping the output, however, that might cause other
    // problems.
    to   = clamp(to,   mn, mx);
    from = clamp(from, mn, mx);
    return true;
  }

  return false;
}

//==============================================================================
template<typename T, typename F>
static void query_ray_helper(const StatGridAABB2<T>& self, vec2 from, vec2 to, F func)
{
  // First, clip the line into the BBOX so we do not run outside
  aabb2 grid_bbox = aabb2{self.m_min, self.m_max - vec2{0.001f}};
  nc_assert(grid_bbox.max != self.m_max);

  bool inside_grid = check_line_in_bbox_and_clip_it(from, to, grid_bbox);
  if (!inside_grid)
  {
    // The line is outside the grid! Exit
    return;
  }

  vec2 abs_dir   = abs(to - from);
  bool swap_axes = abs_dir.x < abs_dir.y;

  ivec2 grid_cnt   = ivec2{self.m_cells.size(), self.m_cells[0].size()};
  vec2  grid_min   = self.m_min;
  vec2  grid_max   = self.m_max;
  vec2  grid_cnt_f = grid_cnt;
  vec2  grid_size  = grid_max - grid_min;

  // Swap axes so that x-change is always greater than y-change
  if (swap_axes)
  {
    std::swap(from.x,       from.y);
    std::swap(to.x,         to.y);
    std::swap(grid_min.x,   grid_min.y);
    std::swap(grid_max.x,   grid_max.y);
    std::swap(grid_cnt_f.x, grid_cnt_f.y);
    std::swap(grid_cnt.x,   grid_cnt.y);
    std::swap(grid_size.x,  grid_size.y);
  }

  // Start cell
  auto[xfrom, yfrom] = grid_helper::remap_point_to_indices_unclamped
  (
    from, grid_min, grid_max, grid_cnt.x, grid_cnt.y
  );

  // End cell
  auto[xto, yto] = grid_helper::remap_point_to_indices_unclamped
  (
    to, grid_min, grid_max, grid_cnt.x, grid_cnt.y
  );

  vec2 direction = to - from;

  if (from.x > to.x)
  {
    // Make sure that from is on the left (TODO: Is this necessary?)
    std::swap(from,  to);
    std::swap(xfrom, xto);
    std::swap(yfrom, yto);
    direction = -direction;
  }

  // Direction has to be checked for scale, it might be 0
  f32 rate       = direction.x ? direction.y / direction.x : 0.0f;
  f32 right_x    = mix(grid_min.x, grid_max.x, (xfrom + 1) / grid_cnt_f.x);
  f32 missing_x  = right_x - from.x;
  f32 missing_y  = missing_x * rate;

  f32 yp = from.y + missing_y;

  // Precalculate here, as we are gonna use it multiple times in a loop
  f32 one_over_gs_y = 1.0f / grid_size.y;
  f32 cell_width    = (grid_size.x / grid_cnt.x);

  for (u64 ix = xfrom, iy = yfrom; ix <= xto; ++ix)
  {
    // Set the correct y for the last iteration. Note that the first iteration
    // can also be the last one.
    if (ix == xto)
    {
      yp = to.y;
    }

    func(swap_axes ? ivec2{iy, ix} : ivec2{ix, iy});

    i64 iy_signed = cast<i64>((yp - grid_min.y) * one_over_gs_y * grid_cnt.y);
    nc_assert(iy_signed >= 0);

    if (u64 new_iy = cast<u64>(iy_signed); new_iy != iy)
    {
      // This happens if the y-coord changes from the previous cell
      func(swap_axes ? ivec2{new_iy, ix} : ivec2{ix, new_iy});
      iy = new_iy;
    }

    // This will be overwritten at the start of the last iteration
    yp += rate * cell_width;
  }
}

//==============================================================================
template<typename T, typename F>
static void query_aabb_helper(const StatGridAABB2<T>& self, aabb2 bbox, F func)
{
  auto[xfrom, yfrom] = grid_helper::remap_point_to_indices
  (
    bbox.min, self.m_min, self.m_max, self.m_cells.size(), self.m_cells[0].size()
  );

  auto[xto, yto] = grid_helper::remap_point_to_indices
  (
    bbox.max, self.m_min, self.m_max, self.m_cells.size(), self.m_cells[0].size()
  );

  for (auto x = xfrom; x <= xto; ++x)
  {
    for (auto y = yfrom; y <= yto; ++y)
    {
      nc_assert(x < self.m_cells.size());
      nc_assert(y < self.m_cells[x].size());

      func(ivec2{x, y});
    }
  }
}

//==============================================================================
u64 ray_count_cells(const StatGridAABB2<u64>& grid, vec2 from, vec2 to)
{
  u64 cnt = 0;
  query_ray_helper(grid, from, to, [&cnt](ivec2 /*coord*/)
  {
    cnt += 1;
  });
  return cnt;
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
  NC_SCOPE_PROFILER(QueryAABB)
  nc_assert(m_initialized);

  grid_helper::query_aabb_helper(*this, bbox, [&](ivec2 coord)
  {
    nc_assert(coord.x < cast<int>(m_cells.size()));
    nc_assert(coord.y < cast<int>(m_cells[coord.x].size()));

    for (auto&[other_bbox, val] : m_cells[coord.x][coord.y])
    {
      if (intersect::aabb_aabb_2d(bbox, other_bbox))
      {
        func(bbox, val);
      }
    }
  });
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_ray
(
  vec3 from, vec3 to, f32 expand, Visitor func
)
const
{
  NC_SCOPE_PROFILER(QueryRay)
  return this->query_ray(from.xz(), to.xz(), expand, func);
}

//==============================================================================
template<typename T>
void StatGridAABB2<T>::query_ray
(
  vec2 from, vec2 to, f32 expand, Visitor func
)
const
{
  nc_assert(m_initialized);
  nc_assert(expand >= 0.0f);
  nc_assert(func);

  // TODO:
  // [ ] handle expanded case

  if (expand > 0.0f)
  {
    // Fallback to stupid implementation for expanded queries.
    vec2 mn = min(from, to);
    vec2 mx = max(from, to);
    vec2 ex = vec2{expand};
    this->query_aabb(aabb2{mn - ex, mx + ex}, func);
  }
  else
  {
    // Faster implementation for raycasts (expansion size == 0.0f)
    grid_helper::query_ray_helper(*this, from, to, [&](ivec2 coord)
    {
      if (coord.x >= this->m_cells.size() || coord.y >= this->m_cells[0].size())
      {
        nc_warn(
          "Out of bounds grid indexing! Grid size: [{}, {}], idx: [{}, {}]",
          this->m_cells.size(), this->m_cells[0].size(), coord.x, coord.y);
        return;
      }

      for (const Unit& unit : this->m_cells[coord.x][coord.y])
      {
        if (intersect::ray_aabb2(from, to, unit.bbox))
        {
          func(unit.bbox, unit.data);
        }
      }
    });
  }
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

#ifdef NC_TESTS
//============================================================================//
//                          FORMATING FOR TESTS                               //
//============================================================================//
template <>
struct std::formatter<nc::ivec2> : std::formatter<std::string>
{
  auto format(const nc::ivec2& v, auto& ctx) const
  {
    return std::formatter<std::string>::format(
      std::format("({}, {})", v.x, v.y),
      ctx
    );
  }
};

//==============================================================================
template <typename Comparator>
struct std::formatter<std::set<nc::ivec2, Comparator>> : std::formatter<std::string>
{
  auto format(const std::set<nc::ivec2, Comparator>& s, auto& ctx) const
  {
    std::string out = "[";
    bool first = true;
    for (const auto& elem : s)
    {
      if (!first)
        out += ", ";
      first = false;
      out += std::format("{}", elem);
    }
    out += "]";
    return std::formatter<std::string>::format(out, ctx);
  }
};

//============================================================================//
//                                   TESTS                                    //
//============================================================================//
// CMD args:
// -unit_test -test_filter=Grid.*
namespace nc
{

//==============================================================================
[[maybe_unused]]auto cmp_ivec2 = [](ivec2 a, ivec2 b)
{
  if (a.x == b.x)
  {
    return a.y < b.y;
  }
  return a.x < b.x;
};

using ResSet = std::set<ivec2, decltype(cmp_ivec2)>;

//==============================================================================
bool grid_test_basic(unit_test::TestCtx& /*ctx*/)
{
  struct TestCase
  {
    vec2   from;
    vec2   to;
    ResSet coords;
  };

  const TestCase TEST_CASES[]
  {
    { vec2{ 0.5f,  0.5f}, vec2{ 0.5f,  0.5f}, ResSet{ ivec2{0, 0}                                       } },
    { vec2{ 0.5f,  0.5f}, vec2{ 1.5f,  0.5f}, ResSet{ ivec2{0, 0}, ivec2{1, 0}                          } },
    { vec2{ 0.5f,  0.5f}, vec2{ 2.5f,  0.5f}, ResSet{ ivec2{0, 0}, ivec2{1, 0}, ivec2{2, 0}             } },
    { vec2{ 0.5f,  0.5f}, vec2{ 0.5f,  1.5f}, ResSet{ ivec2{0, 0}, ivec2{0, 1}                          } },
    { vec2{ 0.5f,  0.5f}, vec2{ 2.5f,  1.5f}, ResSet{ ivec2{0, 0}, ivec2{1, 0}, ivec2{1, 1}, ivec2{2, 1}} },
    { vec2{-0.5f, -0.5f}, vec2{-0.5f, -0.5f}, ResSet{} },
  };

  StatGridAABB2<u64> grid;
  grid.initialize(32, 32, vec2{0, 0}, vec2{32, 32});

  for (const TestCase& c : TEST_CASES)
  {
    for (int permutate = 0; permutate <= 1; ++permutate)
    {
      vec2 from = c.from;
      vec2 to   = c.to;
      if (permutate)
      {
        // Test the swapped from/to, it should produce the same result
        std::swap(from, to);
      }
      
      ResSet res;
      grid_helper::query_ray_helper(grid, from, to, [&](ivec2 coord)
      {
        res.insert(coord);
      });

      if (res != c.coords)
      {
        nc_warn("Grid test failed. Expected: {}. Got: {}", c.coords, res);
        NC_TEST_FAIL;
      }
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(grid_test_basic)->name("Grid Test Basic");

//==============================================================================
bool grid_test_compare(unit_test::TestCtx& /*ctx*/)
{
  StatGridAABB2<u64> grid;
  grid.initialize(32, 29, vec2{1.25f, -0.89f}, vec2{32.3f, 30.0f});

  struct TestCase
  {
    vec2 from;
    vec2 to;
  };

  constexpr TestCase TEST_CASES[]
  {
    { vec2{0.31f, 0.1f } , vec2{3.2f,   3.18f} },
    { vec2{0.6f, 1.8f } , vec2{0.2f,   13.5f} },
    { vec2{5.8f, 13.2f} , vec2{10.15f, 2.8f } },

    // Test cases generated by AI
    { vec2{ 2.0f,  2.0f},   vec2{ 30.0f,  2.0f} }, // horizontal line across
    { vec2{ 5.0f,  5.0f},   vec2{ 5.0f, 25.0f}  }, // vertical line across
    { vec2{ 1.25f, -0.89f}, vec2{ 32.3f, 30.0f} }, // corner-to-corner of the grid AABB
    { vec2{-10.0f, 15.0f},  vec2{ 50.0f, 15.0f} }, // passes fully through grid, outside both sides
    { vec2{10.0f, -20.0f},  vec2{10.0f, 40.0f}  }, // vertical, through grid from outside top & bottom
    { vec2{15.0f, 10.0f},   vec2{15.5f, 10.0f}  }, // tiny horizontal segment inside one cell
    { vec2{25.0f, 5.0f },   vec2{25.0f, 5.0f }  }, // degenerate (from == to)
    { vec2{30.0f, 29.9f},   vec2{35.0f, 29.9f}  }, // just touching upper boundary, then outside
    { vec2{ 0.0f, 30.0f},   vec2{33.0f, -1.0f}  }, // diagonal across, corner → outside
  };

  for (const TestCase c : TEST_CASES)
  {
    ResSet set1, set2;

    // Query the aabb
    grid_helper::query_aabb_helper(grid, aabb2{c.from, c.to}, [&](ivec2 coord)
    {
      vec2  grid_cnt  = vec2{grid.m_cells.size(), grid.m_cells[0].size()};
      vec2  grid_size = grid.m_max - grid.m_min;
      vec2  mn_pt = grid.m_min + grid_size * vec2{coord}            / grid_cnt;
      vec2  mx_pt = grid.m_min + grid_size * vec2{coord + ivec2{1}} / grid_cnt;
      aabb2 bbox  = aabb2{mn_pt, mx_pt};

      if (intersect::ray_aabb2(c.from, c.to, bbox))
      {
        set1.insert(coord);
      }
    });

    // Query ray
    grid_helper::query_ray_helper(grid, c.from, c.to, [&](ivec2 coord)
    {
      set2.insert(coord);
    });

    if (set1 != set2)
    {
      ResSet extra, missing;

      for (ivec2 element : set1)
      {
        if (!set2.contains(element))
        {
          missing.insert(element);
        }
      }

      for (ivec2 element : set2)
      {
        if (!set1.contains(element))
        {
          extra.insert(element);
        }
      }

      nc_warn("Grid test failed. "
              "\nFrom:     ({}, {})"
              "\nTo:       ({}, {})"
              "\nExpected: {}"
              "\nGot:      {}"
              "\nExtra:    {}"
              "\nMissing:  {}\n",
              c.from.x, c.from.y, c.to.x, c.to.y,
              set1, set2, extra, missing);
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(grid_test_compare)->name("Grid Test Compare");

//==============================================================================
// -unit_test -test_filter=Clip.*
bool clip_bbox_test(unit_test::TestCtx& /*ctx*/)
{
  struct TestCase
  {
    aabb2 bbox;
    vec2  from;
    vec2  to;
    vec2  target_from;
    vec2  target_to;
    bool  accept;
  };

  aabb2 unit_bbox = aabb2{VEC2_ZERO, VEC2_ONE};
  aabb2 some_bbox = aabb2{-VEC2_ONE, 2.0f * VEC2_ONE};

  // Test cases generated by AI
  const TestCase TEST_CASES[]
  {
    {unit_bbox, vec2{0.5f, 2.0f},   vec2{0.5f, -2.0f},  vec2{0.5f,  1.0f},  vec2{0.5f,  0.0f},  true},
    {unit_bbox, vec2{0.5f, 1.0f},   vec2{0.5f,  0.0f},  vec2{0.5f,  1.0f},  vec2{0.5f,  0.0f},  true},
    {unit_bbox, vec2{0.25f, 0.25f}, vec2{0.75f, 0.75f}, vec2{0.25f, 0.25f}, vec2{0.75f, 0.75f}, true},

    // Vertical already clipped
    {unit_bbox, vec2{0.5f, 1.0f}, vec2{0.5f,  0.0f}, vec2{0.5f, 1.0f}, vec2{0.5f, 0.0f}, true},

    // Horizontal crossing left–right
    {unit_bbox, vec2{-1.0f, 0.5f}, vec2{ 2.0f, 0.5f}, vec2{ 0.0f, 0.5f}, vec2{ 1.0f, 0.5f}, true},

    // Horizontal entirely above box
    {unit_bbox, vec2{-1.0f, 1.5f}, vec2{ 2.0f, 1.5f}, {}, {}, false},

    // Diagonal top-left to bottom-right
    {unit_bbox, vec2{-1.0f, 2.0f}, vec2{ 2.0f, -1.0f}, vec2{0.0f, 1.0f}, vec2{1.0f, 0.0f}, true},

    // Diagonal bottom-left to top-right
    {unit_bbox, vec2{-1.0f,-1.0f}, vec2{ 2.0f,  2.0f}, vec2{0.0f, 0.0f}, vec2{1.0f, 1.0f}, true},

    // Line fully inside (no clipping needed)
    {unit_bbox, vec2{0.2f, 0.2f}, vec2{0.8f, 0.8f}, vec2{0.2f, 0.2f}, vec2{0.8f, 0.8f}, true},

    // Line entirely outside, parallel to left edge
    {unit_bbox, vec2{-2.0f, 0.2f}, vec2{-1.0f, 0.8f}, {}, {}, false},

    // Line exactly along bottom edge
    {unit_bbox, vec2{-1.0f, 0.0f}, vec2{ 2.0f, 0.0f}, vec2{0.0f, 0.0f}, vec2{1.0f, 0.0f}, true},

    // Crossing with negative bbox (some_bbox)
    {some_bbox, vec2{-3.0f, 0.0f}, vec2{ 3.0f, 0.0f}, vec2{-1.0f, 0.0f}, vec2{ 2.0f, 0.0f}, true},

    // Diagonal crossing some_bbox
    {some_bbox, vec2{-3.0f,-3.0f}, vec2{ 4.0f,  4.0f}, vec2{-1.0f,-1.0f}, vec2{ 2.0f,  2.0f}, true},

    // Completely outside some_bbox
    {some_bbox, vec2{3.0f, 3.5f}, vec2{4.0f, 4.0f}, {}, {}, false},
  };

  for (TestCase c : TEST_CASES)
  {
    bool r = grid_helper::check_line_in_bbox_and_clip_it(c.from, c.to, c.bbox);
    NC_TEST_ASSERT(r == c.accept);

    if (r)
    {
      NC_TEST_ASSERT(c.from == c.target_from);
      NC_TEST_ASSERT(c.to   == c.target_to);
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(clip_bbox_test)->name("Clip Line In BBOX");

}
#endif
