#include <engine/ui/ui_ammo_display.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>
#include <glm/ext/matrix_transform.hpp>

namespace nc
{
	UiAmmoDisplay::UiAmmoDisplay() :
		shader(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
	{
		init();
	}

  UiAmmoDisplay::~UiAmmoDisplay()
  {
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
  }

	void UiAmmoDisplay::init()
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

	void UiAmmoDisplay::update()
	{
		display_ammo = get_engine().get_module<ThingSystem>().get_player()->get_current_weapon_ammo();
    display_health = get_engine().get_module<ThingSystem>().get_player()->get_health();
	}

	void UiAmmoDisplay::draw()
	{
    int ammo = display_ammo;

    if (ammo < 0)
    {
      ammo = 0;
    }

    int health = display_health;

    if (health < 0)
    {
      health = 0;
    }

    std::vector<vec2> positionsAmmo = {vec2(0.8f, -0.8f) , vec2(0.74f, -0.8f), vec2(0.68f, -0.8f)  };
    std::vector<vec2> positionHealth = {vec2(-0.68f, -0.8f), vec2(-0.74f, -0.8f), vec2(-0.8f, -0.8f) };
    vec2 scale = vec2(0.03f, 0.07f);

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager["ui_font"];

    bool first = true;

    shader.use();

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Ammo
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
        digit = 0;
      }
     
      shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
      shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
      shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
      shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
      shader.set_uniform(shaders::ui_text::CHARACTER, digit);

      glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      first = false;
      ammo = ammo / 10;
    }

    // Helath
    for (size_t i = 0; i < 3; i++)
    {
      glm::mat4 trans_mat = glm::mat4(1.0f);
      vec2 translate = positionHealth[i];
      trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
      trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

      const glm::mat4 final_trans = trans_mat;

      int digit = health % 10;
      digit += 48;

      if (first && display_ammo == -1)
      {
        digit = '-';
      }

      if (!first && health == 0)
      {
        digit = 0;
      }

      shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
      shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
      shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
      shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
      shader.set_uniform(shaders::ui_text::CHARACTER, digit);

      glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      first = false;
      health = health / 10;
    }

    

    // unbind
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
	}
}