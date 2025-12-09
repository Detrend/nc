#include <engine/ui/ui_hud_display.h>
#include <engine/core/engine.h>
#include <engine/player/game_system.h>
#include <engine/player/player.h>
#include <glm/ext/matrix_transform.hpp>
#include <engine/graphics/graphics_system.h>

namespace nc
{
  UiHudDisplay::UiHudDisplay() :
    digit_shader(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE),
    text_shader(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE)
  {
    init();
  }

  //=========================================================================================

  UiHudDisplay::~UiHudDisplay()
  {
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
  }

  //=========================================================================================

  void UiHudDisplay::init()
  {
    vec2 vertices[] = { vec2(-1, 1), vec2(0, 0),
        vec2(-1, -1), vec2(0, 1),
        vec2(1, 1), vec2(1, 0),
        vec2(1, -1), vec2(1, 1) };

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // uvs
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  //=========================================================================================

  void UiHudDisplay::update()
  {
    display_ammo = get_engine().get_module<GameSystem>().get_player()->get_current_weapon_ammo();
    display_health = get_engine().get_module<GameSystem>().get_player()->get_health();
  }

  //=========================================================================================

  void UiHudDisplay::draw()
  {
    draw_digits();

    draw_texts();
  }

  //=========================================================================================

  void UiHudDisplay::set_crosshair(int val)
  {
    crosshair = val;
  }

  //=========================================================================================

  void UiHudDisplay::draw_digits()
  {
    digit_shader.use();

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    draw_ammo();
    draw_health();
    draw_crosshair();

    // unbind
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
  }

  void UiHudDisplay::draw_health()
  {
    int health = display_health;

    if (health < 0)
    {
      health = 0;
    }

    bool first = true;

    std::vector<vec2> positionHealth = { vec2(-0.68f, -0.8f), vec2(-0.74f, -0.8f), vec2(-0.8f, -0.8f) };
    vec2 scale = vec2(0.03f, 0.07f);

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager["ui_font"];

    for (size_t i = 0; i < 3; i++)
    {
      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = positionHealth[i];
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

      const glm::mat4 final_trans = trans_mat;

      int digit = health % 10;
      digit += 48;

      if (!first && health == 0)
      {
        digit = 32;
      }

      digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
      digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
      digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
      digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
      digit_shader.set_uniform(shaders::ui_text::CHARACTER, digit);
      digit_shader.set_uniform(shaders::ui_text::HEIGHT, 16.0f);
      digit_shader.set_uniform(shaders::ui_text::WIDTH, 8.0f);

      glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      first = false;
      health = health / 10;
    }
  }

  //=========================================================================================

  void UiHudDisplay::draw_ammo()
  {
    int ammo = display_ammo;

    if (ammo < 0)
    {
      ammo = 0;
    }

    bool first = true;

    std::vector<vec2> positionsAmmo = { vec2(0.8f, -0.8f) , vec2(0.74f, -0.8f), vec2(0.68f, -0.8f) };
    vec2 scale = vec2(0.03f, 0.07f);

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager["ui_font"];

    for (size_t i = 0; i < 3; i++)
    {
      glActiveTexture(GL_TEXTURE0);

      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = positionsAmmo[i];
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

      const glm::mat4 final_trans = trans_mat;

      int digit = ammo % 10;
      digit += 48;

      if (first && display_ammo == -1)
      {
        digit = '-';
      }

      if (!first && ammo == 0)
      {
        digit = 32;
      }

      digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
      digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
      digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
      digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
      digit_shader.set_uniform(shaders::ui_text::CHARACTER, digit);
      digit_shader.set_uniform(shaders::ui_text::HEIGHT, 16.0f);
      digit_shader.set_uniform(shaders::ui_text::WIDTH, 8.0f);

      glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      first = false;
      ammo = ammo / 10;
    }
  }

  //=====================================================================================

  void UiHudDisplay::draw_texts()
  {
    // Drawing texts under the numbers

    std::vector<vec2> positionsTexts = { vec2(-0.74f, -0.9f), vec2(0.74f, -0.9f) };
    std::vector<vec2> scalesTexts = { vec2(0.09f, 0.035f), vec2(0.06f, 0.035f) };
    std::vector<const char*> texts = { "ui_health", "ui_ammo" };

    text_shader.use();

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < 2; i++)
    {
      // transformation matrices
      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = positionsTexts[i];
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      trans_mat = glm::scale(trans_mat, glm::vec3(scalesTexts[i].x, scalesTexts[i].y, 1));

      const glm::mat4 final_trans = trans_mat;

      // texture pickup
      const TextureManager& manager2 = TextureManager::get();
      const TextureHandle& texture2 = manager2[texts[i]];

      text_shader.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
      text_shader.set_uniform(shaders::ui_button::ATLAS_SIZE, texture2.get_atlas().get_size());
      text_shader.set_uniform(shaders::ui_button::TEXTURE_POS, texture2.get_pos());
      text_shader.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture2.get_size());

      glBindTexture(GL_TEXTURE_2D, texture2.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
  }

  void UiHudDisplay::draw_crosshair()
  {

    vec2 win_size = get_engine().get_module<GraphicsSystem>().get_window_size();

    vec2 position = vec2(0.0f, 0.0f);
    vec2 scale = vec2(0.05f, 0.05f * win_size.x / win_size.y);

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager["ui_crosshair"];

    glActiveTexture(GL_TEXTURE0);

    glm::mat4 trans_mat = glm::mat4(1.0f);
    vec2 translate = position;
    trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
    trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

    const glm::mat4 final_trans = trans_mat;
    digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
    digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
    digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
    digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());

    digit_shader.set_uniform(shaders::ui_text::CHARACTER, crosshair);

    digit_shader.set_uniform(shaders::ui_text::HEIGHT, 1.0f);
    digit_shader.set_uniform(shaders::ui_text::WIDTH, 10.0f);

    glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}