// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <functional>

namespace nc
{

class  IEntityRenderProxy;
class  SectorsRenderProxy;
class  RenderSettings;
struct LevelStats;

using EntityRenderLambda         = std::function<void(IEntityRenderProxy* proxy, f32 interp_coeff)>;
using SectorsRenderLambda        = std::function<void(SectorsRenderProxy* proxy, f32 interp_coeff)>;
using LevelStatsRenderLambda     = std::function<void(LevelStats& stats)>;
using RenderSettingsRenderLambda = std::function<void(RenderSettings& settings)>;

}
