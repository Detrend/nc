#include "tracing.shared.h"

IMAGE(RWTexture2D<float3>, out_color);

[shader("compute")]
[numthreads(TRACING_GROUP_SIZE, TRACING_GROUP_SIZE, 1)]
void main(uvec2 pixel: SV_DispatchThreadID)
{
  uvec2 resolution;
  out_color.GetDimensions(resolution.x, resolution.y);
  if (any(pixel >= resolution)) return;

  out_color[pixel] = float3(1.0f, 0.0f, 0.0f);
}
