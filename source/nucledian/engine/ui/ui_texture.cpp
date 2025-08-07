#include <engine/ui/ui_texture.h>

namespace nc
{
  GuiTexture::GuiTexture(int texture, vec2 position, vec2 scale)
  {
    this->texture = texture;
    this->position = position;
    this->scale = scale;
  }

  //=========================================================

  int GuiTexture::get_texture()
  {
    return texture;
  }

  //=========================================================

  vec2 GuiTexture::get_position()
  {
    return position;
  }

  //========================================================

  vec2 GuiTexture::get_scale()
  {
    return scale;
  }
}