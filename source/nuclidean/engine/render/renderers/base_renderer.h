// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
#include <math/vector.h>
#include <engine/map/map_types.h>

#include <memory>

namespace nc::render::renderers
{

class BaseRenderer
{
public:
  virtual ~BaseRenderer() = default;

  virtual bool init() { return true; }
  virtual void terminate() {}

  // Called when the size of the window in pixels changed.
  virtual void on_window_resolution_changed([[maybe_unused]] uvec2 window_resolution) {}
  // Called when the resolution of the image the game renders into changed.
  virtual void on_virtual_resolution_changed([[maybe_unused]] uvec2 virtual_resolution) {}
  // Called after a level is loaded and its map is built.
  virtual void on_map_rebuilt() {}
  // Called when the sector floor or ceiling moved this frame.
  virtual void on_sector_changed([[maybe_unused]] SectorID sector_id) {}
  // Called when the wall segment switched between its default and triggered texture.
  virtual void on_segment_changed([[maybe_unused]] SegmentID segment_id) {}
  virtual void render() = 0;

#if NC_DEBUG_DRAW
  // GPU time of rendering a recent frame, in milliseconds.
  virtual f32 get_gpu_time() const { return 0.0f; }
#endif

#if NC_HOT_RELOAD
  // Called every frame, recompiles shaders whose sources changed, keeping the old ones on failure.
  virtual void reload_shaders() {}
#endif

};
using BaseRendererPtr = std::unique_ptr<BaseRenderer>;

}
