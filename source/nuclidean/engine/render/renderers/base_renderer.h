// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
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

  virtual void on_window_resized([[maybe_unused]] u32 width, [[maybe_unused]] u32 height) {}
  // Called after a level is loaded and its map is built.
  virtual void on_map_rebuilt() {}
  // Called when the sector floor or ceiling moved this frame.
  virtual void on_sector_changed([[maybe_unused]] SectorID sector_id) {}
  // Called when the wall segment switched between its default and triggered texture.
  virtual void on_segment_changed([[maybe_unused]] SegmentID segment_id) {}
  virtual void render() = 0;

#if NC_HOT_RELOAD
  // Called every frame, recompiles shaders whose sources changed, keeping the old ones on failure.
  virtual void reload_shaders() {}
#endif

};
using BaseRendererPtr = std::unique_ptr<BaseRenderer>;

}
