// Project Nuclidean Source File
#include <engine/ui/ui_button.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/resources/texture.h>
#include <engine/core/module_event.h>
#include <engine/ui/user_interface_system.h>

namespace nc 
{
UiButton::UiButton()
{
}

//=============================================================================================
UiButton::UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func)
{
  this->texture_name = texture_name;

  this->position = position;
  this->scale = scale;
  this->func = std::move(func);

}

//================================================================================================
bool UiButton::is_point_in_rec(vec2 point)
{
  vec2 start = position - scale;
  vec2 end = position + scale;

  // x coords
  if (point.x < start.x || point.x > end.x)
  {
    return false;
  }

  // y coords
  if (point.y < start.y || point.y > end.y)
  {
    return false;
  }

  return true;
}

//===================================================================================================
vec2 UiButton::get_position()
{
  return position;
}

//======================================================================================================
vec2 UiButton::get_scale()
{
  return scale;
}

//==============================================================================================
void UiButton::set_hover(bool hover)
{
  isHover = hover;
}

//==============================================================================================
void UiButton::on_click()
{
  SoundSystem::get().play_oneshot(Sounds::ui_click, 1.0f, SoundLayers::ui);
  func();
}

//====================================================================================================
void UiButton::draw(UiDrawList& out) const
{
  const TextureHandle& texture = TextureManager::get()[texture_name];
  out.add_image(texture, position, scale, isHover);
}

//==========================================================================================
UiLoadGameButton::UiLoadGameButton(const std::string& save, vec2 position, vec2 scale)
: save_path(save)
{
  this->position = position;
  this->scale    = scale;
}

//=============================================================================================
void UiLoadGameButton::on_click()
{
  get_engine().send_event(ModuleEvent
  {
    .type = ModuleEventType::new_game_level_requested,
  });

  GameSystem::get().load_game(SAVE_DIR_RELATIVE + ("/" + save_path));
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_visible(false);

}

//==============================================================================================
void UiLoadGameButton::draw(UiDrawList& out) const
{
  vec2 step       = vec2(0.033f, 0.0f);
  vec2 char_scale = vec2(0.0165f, 0.033f);

  const TextureHandle& font = TextureManager::get()["ui_font"];

  vec2 pos = position - vec2(0.5f, 0.0f);
  for (char c : save_path)
  {
    out.add_char(font, pos, char_scale, cast<s32>(c), isHover);
    pos += step;
  }
}
}
