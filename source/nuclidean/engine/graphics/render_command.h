// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <functional>

namespace nc
{

class IEntityRenderProxy;
class SectorsRenderProxy;

using EntityRenderLambda  = std::function<void(IEntityRenderProxy* proxy, f32 interp_coeff)>;
using SectorsRenderLambda = std::function<void(SectorsRenderProxy* proxy, f32 interp_coeff)>;

}
