// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>

namespace nc
{
struct UiDrawList;
class  TextureHandle;

class UiHudDisplay
{
public:
  void update(float delta_time);

  void show_secret();
  void show_saved();

  // records the HUD into the UI draw list
  void draw(UiDrawList& out) const;

  void set_crosshair(int val);

private:
  void draw_number(UiDrawList& out, const TextureHandle& font, int value, vec2 first_pos, vec2 step) const;
  void draw_health(UiDrawList& out) const;
  void draw_ammo(UiDrawList& out) const;
  void draw_texts(UiDrawList& out) const;
  void draw_crosshair(UiDrawList& out) const;
  void draw_secret_revealed(UiDrawList& out) const;
  void draw_saved(UiDrawList& out) const;

  int display_ammo = 0;
  int display_health = 0;
  int crosshair = 1;

  static constexpr float TIME_TO_SHOW_SECRET = 3.0f;

  float time_since_secret = TIME_TO_SHOW_SECRET + 1.0f;
  float time_since_saved = TIME_TO_SHOW_SECRET + 1.0f;
};
}
