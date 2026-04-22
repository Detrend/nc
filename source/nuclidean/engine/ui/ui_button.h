#pragma once

#include <engine/graphics/graphics_system.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/texture.h>
#include <engine/game/game_system.h>

#include <vector>
#include <functional>
#include <string>

//======================================================================================

namespace nc
{
  class UiButton
  {
  public:
    UiButton();
    UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func);

    //checks overlap of point and button
    bool is_point_in_rec(vec2 point);

    vec2 get_position();
    vec2 get_scale();

    void set_hover(bool hover);

    // action to be called when a button is pressed
    virtual void on_click();

    // render the button
    // draw takes the shader to modify its uniforms
    // VAO must be bound before this is called
    virtual void draw(ShaderProgramHandle button_material);

  protected:
    const char* texture_name;
    vec2 position;
    vec2 scale;
    bool isHover = false;

  private:
    std::function<void(void)> func;
  };
  //======================================================================================

  class UiLoadGameButton : public UiButton
  {
  public:
    UiLoadGameButton(nc::GameSystem::SaveDbEntry& save_entry, vec2 position, vec2 scale);

    // action to be called when a button is pressed
    void on_click() override;

    // render the button
    // draw takes the shader to modify its uniforms
    // VAO must be bound before this is called
    void draw(ShaderProgramHandle digit_material);

  private:
    nc::GameSystem::SaveDbEntry& save;
  };

}