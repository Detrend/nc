#include <engine/ui/ui_texture.h>
#include <glad/glad.h>

#include <vector>
#include <functional>

namespace nc
{
  class UiButton
  {
  public:
    UiButton(GuiTexture* texture, std::function<void(void)> func);
    bool is_point_in_rec(vec2 point);


  private:
    GuiTexture* texture;
    bool isHover = false;
    std::function<void(void)> func;
  };

  class MainMenuPage
  {

  };

  class OptionsPage
  {

  };

  class SaveGamePage 
  {

  };

  class LoadGamePage
  {

  };

  

  class MenuManager
  {

  };
}