// Project Nucledian Source File
#include <grid.h>
#include <common.h>

#include <math/vector.h>
#include <math/lingebra.h>
#include <intersect.h>

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
  f32 mxx = max.x - 0.0001f;
  f32 mxy = max.y - 0.0001f;
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
template<typename T, typename F>
static void query_ray_helper(const StatGridAABB2<T>& self, vec2 from, vec2 to, F func)
{
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
  }

  // Start cell
  auto[xfrom, yfrom] = grid_helper::remap_point_to_indices
  (
    from, grid_min, grid_max, grid_cnt.x, grid_cnt.y
  );

  // End cell
  auto[xto, yto] = grid_helper::remap_point_to_indices
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

  f32 rate       = direction.y / direction.x;
  f32 right_x    = mix(grid_min.x, grid_max.x, (xfrom + 1) / grid_cnt_f.x);
  f32 missing_x  = right_x - from.x;
  f32 missing_y  = missing_x * rate;

  f32 yp = from.y + missing_y;

  // Precalculate here, as we are gonna use it multiple times in a loop
  f32 one_over_gs_y = 1.0f / grid_size.y;

  for (u64 ix = xfrom, iy = yfrom; ix <= xto; ++ix)
  {
    func(swap_axes ? ivec2{iy, ix} : ivec2{ix, iy});

    u64 new_iy = cast<u64>((yp - grid_min.y) * one_over_gs_y * grid_cnt.y);
    if (new_iy != iy)
    {
      // This happens if the y-coord changes from the previous cell
      func(swap_axes ? ivec2{new_iy, ix} : ivec2{ix, new_iy});
    }

    iy = new_iy;

    yp += rate;

    if (ix + 1 == xto) [[unlikely]] // Before the last iteration
    {
      // Adjust the y so we do not overshoot behind the to point
      f32 rate_or_zero = rate ? rate : 0.0f;
      f32 rem_x        = mix(grid_min.x, grid_max.x, (xto + 1) / grid_cnt_f.x);
      f32 miss_x       = rem_x - to.x;
      f32 miss_y       = miss_x * rate_or_zero;
      yp -= miss_y; 
    }
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
    // Faster implementation for raycasts (expansion size = 0.0f)
    grid_helper::query_ray_helper(*this, from, to, [&](ivec2 coord)
    {
      for (const Unit& unit : this->m_cells[coord.x][coord.y])
      {
        func(unit.bbox, unit.data);
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
    { vec2{-0.5f, -0.5f}, vec2{-0.5f, -0.5f}, ResSet{ ivec2{0, 0}                                       } },
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
  grid.initialize(32, 32, vec2{0, 0}, vec2{32, 32});

  struct TestCase
  {
    vec2 from;
    vec2 to;
  };

  constexpr TestCase TEST_CASES[]
  {
    { vec2{0.3f, 0.1f } , vec2{3.2f,   3.18f} },
    { vec2{0.6f, 1.8f } , vec2{0.2f,   13.5f} },
    { vec2{5.8f, 13.2f} , vec2{10.15f, 2.8f } },
  };

  for (const TestCase c : TEST_CASES)
  {
    ResSet set1, set2;

    // Query the aabb
    grid_helper::query_aabb_helper(grid, aabb2{c.from, c.to}, [&](ivec2 coord)
    {
      vec2  grid_cnt  = vec2{grid.m_cells.size(), grid.m_cells[0].size()};
      vec2  grid_size = grid.m_max - grid.m_min;
      vec2  mn_pt = (vec2{coord}              / grid_cnt) * grid_size + grid.m_min;
      vec2  mx_pt = (vec2{(coord + ivec2{1})} / grid_cnt) * grid_size + grid.m_min;
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
              "\nExpected: {}"
              "\nGot:      {}"
              "\nExtra:    {}"
              "\nMissing:  {}\n",
              set1, set2, extra, missing);
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(grid_test_compare)->name("Grid Test Compare");

}
#endif
