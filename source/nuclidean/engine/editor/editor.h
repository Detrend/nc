// Project Nuclidean Source File
#pragma once

#include <types.h>

namespace nc
{

class Editor
{
public:
  static Editor* get();

  void init();
  void terminate();
  void update(f32 delta);
  void render();
};

}
