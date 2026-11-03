// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <functional>

namespace nc
{

class IEntityRenderProxy;

using EntityRenderLambda = std::function<void(IEntityRenderProxy* proxy, f32 interp_coeff)>;

}
