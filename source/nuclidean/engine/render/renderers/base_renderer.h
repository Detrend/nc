// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>

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
  virtual void render() = 0;

#if NC_HOT_RELOAD
  // Called every frame, recompiles shaders whose sources changed, keeping the old ones on failure.
  virtual void reload_shaders() {}
#endif

};
using BaseRendererPtr = std::unique_ptr<BaseRenderer>;

}
