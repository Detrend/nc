// Project Nuclidean Source File

#include <common.h>
#include <config.h>

#if NC_PROFILING

#include <engine/game/game_system.h>
#include <engine/map/map_system.h>
#include <engine/map/map_dynamics.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_system.h>
#include <engine/player/level_types.h>

#include <math/vector.h>

#include <benchmark/benchmark.h>
#include <json/json.hpp>
#include <vector>
#include <random>

namespace nc
{

//==================================================================================================
static void benchmark_map_json_parse(benchmark::State& state, LevelName level)
{
  for (auto _ : state)
  {
    nlohmann::json data;
    map_helpers::load_and_parse_map_json(level, data);
    benchmark::DoNotOptimize(data);
  }
}

//==================================================================================================
static void benchmark_map_load(benchmark::State& state, LevelName level)
{
  for (auto _ : state)
  {
    MapSectors     map;
    SectorMapping  mapping{map};
    EntityRegistry registry;
    MapDynamics    dynamics{map, registry, mapping};
    EntityID       player_id;

    map_helpers::load_json_map_partial(level, map, mapping, registry, dynamics, player_id);

    benchmark::DoNotOptimize(map);
    benchmark::DoNotOptimize(mapping);
    benchmark::DoNotOptimize(registry);
    benchmark::DoNotOptimize(dynamics);
    benchmark::DoNotOptimize(player_id);
  }
}

//==================================================================================================
static f32 random_pt(f32 mn, f32 mx)
{
  constexpr int MX = 1024 * 8;
  int r = std::rand();

  f64 zero_to_one = (r % MX) / cast<f64>(MX-1);
  f64 num_64      = cast<f64>(mx) * zero_to_one + cast<f64>(mn) * (1.0 - zero_to_one);

  return cast<f32>(num_64);
}

//==================================================================================================
static void setup_map_and_points
(
  MapSectors& map, LevelName level, std::vector<vec2>& points_to_sample, u32 points_per_m2
)
{
  SectorMapping  mapping{map};
  EntityRegistry registry;
  MapDynamics    dynamics{map, registry, mapping};
  EntityID       player_id;

  map_helpers::load_json_map_partial(level, map, mapping, registry, dynamics, player_id);

  // iterate all sectors and choose random points within them
  for (SectorID sid = 0; sid < map.sectors.size(); ++sid)
  {
    aabb3 bbox3 = map.sector_bboxes[sid];
    vec3  size  = bbox3.max - bbox3.min;
    f32   area  = size.x * size.z;

    u64 num_pts = cast<u64>(std::ceil(area / cast<f32>(points_per_m2)));

    for (u64 i = 0; i < num_pts; ++i)
    {
      f32 x = random_pt(bbox3.min.x, bbox3.max.x);
      f32 y = random_pt(bbox3.min.z, bbox3.max.z);
      points_to_sample.push_back(vec2{x, y});
    }
  }
}

//==================================================================================================
static void benchmark_sector_from_point(benchmark::State& state, LevelName level, u32 points_per_m2)
{
  std::vector<vec2> points_to_sample;
  MapSectors map;

  setup_map_and_points(map, level, points_to_sample, points_per_m2);

  // now sample the points from the map
  for (auto _ : state)
  {
    for (vec2 pt : points_to_sample)
    {
      SectorID sid = map.get_sector_from_point(pt);
      benchmark::DoNotOptimize(sid);
    }
  }

  state.SetItemsProcessed(cast<s64>(state.iterations() * points_to_sample.size()));
}

BENCHMARK_CAPTURE(benchmark_map_json_parse, ParseJsonLevel1, Levels::LEVEL_1)->Unit(benchmark::kSecond);
BENCHMARK_CAPTURE(benchmark_map_json_parse, ParseJsonLevel2, Levels::LEVEL_2)->Unit(benchmark::kSecond);

BENCHMARK_CAPTURE(benchmark_map_load, LoadLevel1, Levels::LEVEL_1)->Unit(benchmark::kSecond);
BENCHMARK_CAPTURE(benchmark_map_load, LoadLevel2, Levels::LEVEL_2)->Unit(benchmark::kSecond);

BENCHMARK_CAPTURE(benchmark_sector_from_point, SectorFromPointLvl1, Levels::LEVEL_1, 4)->Unit(benchmark::kMicrosecond);

}

#endif
