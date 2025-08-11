#include <engine/ui/ui_texture.h>

#include <vector>
#include <functional>

namespace nc
{
  class UiButton
  {
  public:
    UiButton(GuiTexture* texture, std::function<void> func);
    bool is_point_in_rec(vec2 point);


  private:
    GuiTexture* texture;
    bool isHover = false;
    std::function<void> func;
  };

  class UiButtonManager
  {
  public:
  private:
    std::vector<UiButton> buttons;
  };
}