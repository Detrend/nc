#include <engine/ui/ui_screen_effect.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nc
{

UiScreenEffect::UiScreenEffect():
	shader(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE)
{
  init();
}

void UiScreenEffect::did_damage([[maybe_unused] ]int damage)
{
  damage *= 2;
  time_since_last_dmg = time_since_last_dmg - damage / 100.0f;
  //time_since_last_dmg = 0.0f;
}

void UiScreenEffect::did_pickup()
{
  time_since_last_pickup = 0.0f;
}

void UiScreenEffect::init()
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

void UiScreenEffect::update(float delta)
{
  time_since_last_dmg = min(time_since_last_dmg + delta, MAX_DMG_FLASH_DURATION);
  time_since_last_pickup = min(time_since_last_pickup + delta, MAX_PICKUP_FLASH_DURATION);
}

void UiScreenEffect::draw()
{
  shader.use();

  glBindVertexArray(VAO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  const TextureManager& manager = TextureManager::get();
  const TextureHandle& texture = manager["ui_screen_effect"];

  glActiveTexture(GL_TEXTURE0);

  glm::mat4 trans_mat = glm::mat4(1.0f);
  trans_mat = glm::translate(trans_mat, glm::vec3(0, 0, 0));
  trans_mat = glm::scale(trans_mat, glm::vec3(1, 1, 1));

  const glm::mat4 final_trans = trans_mat;

  shader.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
  shader.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas().get_size());
  shader.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
  shader.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());

  if ((MAX_DMG_FLASH_DURATION - time_since_last_dmg) == 0)
  {
    shader.set_uniform(shaders::ui_button::COLOR, vec4(1.0f, 1.0f, 0.0f, (MAX_PICKUP_FLASH_DURATION - time_since_last_pickup)/ MAX_PICKUP_FLASH_DURATION));
  }
  else
  {
    shader.set_uniform(shaders::ui_button::COLOR, vec4(1.0f, 0.0f, 0.0f, (MAX_DMG_FLASH_DURATION - time_since_last_dmg)));
  }

  glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindVertexArray(0);
}

}


