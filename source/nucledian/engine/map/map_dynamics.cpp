// Project Nucledian Source File

#include <engine/map/map_dynamics.h>
#include <engine/map/map_system.h>

#include <math/lingebra.h>
#include <math/utils.h>

namespace nc
{

//==============================================================================
static void lerp_towards(f32& value, f32 target, f32 amount)
{
  f32 rem = target - value;
  f32 dir = sgn(rem);
  f32 change = dir * min(abs(rem), amount);
  value += change;
}

//==============================================================================
void MapDynamics::update(f32 delta)
{
  for (const auto& item : entries)
  {
    const SectorID     sid   = item.first;
    const SectorEntry& entry = item.second;
    const SectorState& desired = entry.states[entry.state];

    SectorData& sd = map.sectors[sid];

    bool changed = false;

    f32 change_per_frame = delta * entry.speed;

    if (sd.floor_height != desired.floor_height && change_per_frame != 0)
    {
      lerp_towards(sd.floor_height, desired.floor_height, change_per_frame);
      changed = true;
    }

    if (sd.ceil_height != desired.ceil_height && change_per_frame != 0)
    {
      lerp_towards(sd.ceil_height, desired.ceil_height, change_per_frame);
      changed = true;
    }

    if (changed && callback)
    {
      callback(sid);
    }
  }
}

}
