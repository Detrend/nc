// Project Nuclidean Source File
#pragma once

#include <engine/ui/ui_button.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/core/module_event.h>
#include <engine/ui/user_interface_system.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    this->func = func;

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

  void UiButton::draw([[maybe_unused]] const ShaderProgramHandle button_material)
  {
    button_material.use();

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager[texture_name];

    glActiveTexture(GL_TEXTURE0);

    // calculate uniforms
    glm::mat4 trans_mat = glm::mat4(1.0f);
    vec2 translate = position;
    trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
    trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

    const glm::mat4 final_trans = trans_mat;

    // set shader uniforms
    button_material.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
    button_material.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas().get_size());
    button_material.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
    button_material.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());
    button_material.set_uniform(shaders::ui_button::HOVER, isHover);

    glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  //==========================================================================================

  UiLoadGameButton::UiLoadGameButton(nc::GameSystem::SaveDbEntry& save_entry, vec2 position, vec2 scale) :
    save(save_entry)
  {
    this->position = position;
    this->scale = scale;

  }

  //=============================================================================================
  void UiLoadGameButton::on_click()
  {
    get_engine().send_event(ModuleEvent
      {
        .type = ModuleEventType::new_game_level_requested,
      });
    get_engine().get_module<GameSystem>().load_game(save.data);
    get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_visible(false);

  }

  //==============================================================================================

  void UiLoadGameButton::draw(ShaderProgramHandle digit_material)
  {
    // draw button, the text is save time
    nc::SaveGameData::SaveTime time = save.data.time;

    std::string text = std::format("{:%Y-%m-%d %H:%M}", time);

    vec2 final_pos = position - vec2(0.5f, 0.0f);
    vec2 step = vec2(0.033f, 0.0f);

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager["ui_font"];

    for (size_t c = 0; c < text.size(); c++)
    {
      glActiveTexture(GL_TEXTURE0);

      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = final_pos;
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      trans_mat = glm::scale(trans_mat, glm::vec3(0.0165f, 0.033f, 1));

      const glm::mat4 final_trans = trans_mat;

      int digit = int(text[c]);

      //set uniforms
      digit_material.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
      digit_material.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
      digit_material.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
      digit_material.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
      digit_material.set_uniform(shaders::ui_text::CHARACTER, digit);
      digit_material.set_uniform(shaders::ui_text::HEIGHT, 16);
      digit_material.set_uniform(shaders::ui_text::WIDTH, 8);
      digit_material.set_uniform(shaders::ui_text::HOVER, isHover);

      glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      final_pos += step;
    }
  }
}