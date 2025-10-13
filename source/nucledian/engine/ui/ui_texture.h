#pragma once

#include <math/vector.h>


namespace nc
{
  class GuiTexture
  {
  public:
    GuiTexture(int texture, vec2 position, vec2 scale);

    int get_texture();
    vec2 get_position();
    vec2 get_scale();


  private:
    int texture;
    vec2 position;
    vec2 scale;
  };
}