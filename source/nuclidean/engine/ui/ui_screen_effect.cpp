// Project Nuclidean Source File
#include <engine/ui/ui_screen_effect.h>

#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/resources/texture.h>

#include <glm/common.hpp>

namespace nc
{

//===========================================================================================
void UiScreenEffect::did_damage(int damage)
{
// final color is determined by how big the damage is
damage *= 2;
time_since_last_dmg = time_since_last_dmg - damage / 100.0f;
}

//===========================================================================================
void UiScreenEffect::did_pickup()
{
// resets pickup timer
time_since_last_pickup = 0.0f;
}

//===========================================================================================
void UiScreenEffect::clear_effect()
{
  time_since_last_dmg = MAX_DMG_FLASH_DURATION;
  time_since_last_pickup = MAX_PICKUP_FLASH_DURATION;
}

//===========================================================================================
void UiScreenEffect::update(float delta)
{
time_since_last_dmg = min(time_since_last_dmg + delta, MAX_DMG_FLASH_DURATION);
time_since_last_pickup = min(time_since_last_pickup + delta, MAX_PICKUP_FLASH_DURATION);
}

//===========================================================================================
void UiScreenEffect::draw(UiDrawList& out) const
{
  const TextureHandle& texture = TextureManager::get()["ui_screen_effect"];

  UiQuad& quad = out.add_image(texture, VEC2_ZERO, VEC2_ONE);
  quad.uvs     = UiQuadUvs::full;

  //determine color of flash -> red (damage) has priority
  if ((MAX_DMG_FLASH_DURATION - time_since_last_dmg) == 0)
  {
    quad.color = vec4(1.0f, 1.0f, 0.0f, (MAX_PICKUP_FLASH_DURATION - time_since_last_pickup)/ MAX_PICKUP_FLASH_DURATION);
  }
  else
  {
    quad.color = vec4(1.0f, 0.0f, 0.0f, (MAX_DMG_FLASH_DURATION - time_since_last_dmg));
  }
}

}
