#include <engine/ui/ui_button.h>

namespace nc
{
  UiButton::UiButton(GuiTexture* texture, std::function<void(void)> func)
  {
    this->texture = texture;
    this->func = func;
  }

  bool UiButton::is_point_in_rec(vec2 point)
  {
    vec2 position = texture->get_position();
    vec2 scale = texture->get_scale();

    vec2 start = position - scale / 2.0f;
    vec2 end = position + scale / 2.0f;

    if (point.x < start.x || point.x > end.x)
    {
      return false;
    }

    if (point.y < start.y || point.y > end.y)
    {
      return false;
    }

    return true;
  }
}