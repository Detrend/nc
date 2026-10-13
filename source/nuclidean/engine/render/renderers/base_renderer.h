// Project Nuclidean Source File
#pragma once

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

};
using BaseRendererPtr = std::unique_ptr<BaseRenderer>;

}
