// Project Nuclidean Source File
#include <engine/ui/ui_hud_display.h>
#include <engine/core/engine.h>
#include <engine/player/player.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/resources/texture.h>

namespace nc
{

static constexpr vec2 HUD_DIGIT_SCALE = vec2(0.03f, 0.07f);

//=========================================================================================
void UiHudDisplay::update(float delta_time)
{
  // Player might not exist in an empty level in menu
  if (Player* player = GameHelpers::get().get_player())
  {
    display_ammo   = player->get_current_weapon_ammo();
    display_health = player->get_health();
    time_since_secret += delta_time;
    time_since_saved += delta_time;
  }
  else
  {
    display_ammo   = 0;
    display_health = 0;
  }
}

//=========================================================================================
void UiHudDisplay::draw(UiDrawList& out) const
{
  draw_ammo(out);
  draw_health(out);
  draw_crosshair(out);
  draw_texts(out);
}

//=========================================================================================
void UiHudDisplay::set_crosshair(int val)
{
  crosshair = val;
}

//=========================================================================================
// Draws 3 digits from right to left, starting at the first_pos. Leading zeroes
// are drawn as spaces, but at least one digit is always drawn.
void UiHudDisplay::draw_number
(
  UiDrawList&          out,
  const TextureHandle& font,
  int                  value,
  vec2                 first_pos,
  vec2                 step
) const
{
  bool first = true; // to always render a char

  vec2 pos = first_pos;
  for (size_t i = 0; i < 3; i++)
  {
    int digit = value % 10 + '0';

    if (!first && value == 0)
    {
      digit = ' ';
    }

    out.add_char(font, pos, HUD_DIGIT_SCALE, digit);

    first = false;
    value = value / 10;
    pos  += step;
  }
}

//===========================================================================================
void UiHudDisplay::draw_health(UiDrawList& out) const
{
  // draw health from right to left
  const TextureHandle& font = TextureManager::get()["ui_font"];
  draw_number(out, font, max(display_health, 0), vec2(-0.68f, -0.78f), vec2(-0.06f, 0.0f));
}

//=========================================================================================
void UiHudDisplay::draw_ammo(UiDrawList& out) const
{
  const TextureHandle& font = TextureManager::get()["ui_font"];

  size_t first_quad = out.quads.size();
  draw_number(out, font, max(display_ammo, 0), vec2(0.8f, -0.78f), vec2(-0.06f, 0.0f));

  if (display_ammo == -1) //if melee, we don't want to use ammo count
  {
    out.quads[first_quad].character = '-';
  }
}

//=====================================================================================
void UiHudDisplay::draw_texts(UiDrawList& out) const
{
  // Drawing texts under the numbers
  const TextureManager& manager = TextureManager::get();

  out.add_image(manager["ui_health"], vec2(-0.74f, -0.9f), vec2(0.09f, 0.035f));
  out.add_image(manager["ui_ammo"],   vec2( 0.74f, -0.9f), vec2(0.06f, 0.035f));

  draw_secret_revealed(out);
  draw_saved(out);
}

//===========================================================================================
void UiHudDisplay::draw_crosshair(UiDrawList& out) const
{
  vec2 win_size = get_engine().get_module<GraphicsSystem>().get_window_size_game_thread();
  vec2 scale    = vec2(0.05f, 0.05f * win_size.x / win_size.y);

  // the crosshair texture is a grid of 11x2 crosshairs
  const TextureHandle& texture = TextureManager::get()["ui_crosshair"];
  out.add_glyph(texture, VEC2_ZERO, scale, crosshair + 11, 11, 2);
}

//===========================================================================================
void UiHudDisplay::draw_secret_revealed(UiDrawList& out) const
{
  if (time_since_secret > TIME_TO_SHOW_SECRET)
  {
    return;
  }

  out.add_image(TextureManager::get()["ui_secret_revealed"], vec2(0.0f, -0.5f), vec2(0.275f, 0.04f));
}

//==========================================================================
void UiHudDisplay::draw_saved(UiDrawList& out) const
{
  if (time_since_saved > TIME_TO_SHOW_SECRET)
  {
    return;
  }

  out.add_image(TextureManager::get()["ui_saved"], vec2(0.0f, -0.6f), vec2(0.275f, 0.04f));
}

//===========================================================================================
void UiHudDisplay::show_secret()
{
  time_since_secret = 0.0f;
}

//===========================================================================================
void UiHudDisplay::show_saved()
{
  time_since_saved = 0.0f;
}
}
