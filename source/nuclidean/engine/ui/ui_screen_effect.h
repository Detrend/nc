// Project Nuclidean Source File
#pragma once
#include <engine/graphics/resources/shader_program.h>

namespace nc
{
class UiScreenEffect
{
  public:
    UiScreenEffect();
      
    void update(float delta);

    void draw();

    // call when player receives damage
    void did_damage(int damage);

    // call when player picks up an item
    void did_pickup();

  private:
      
    void init();

    // consts
    const f32 MAX_DMG_FLASH_DURATION = 2.0f;
    const f32 MAX_PICKUP_FLASH_DURATION = 0.25f;

    // properties
    f32 time_since_last_dmg = MAX_DMG_FLASH_DURATION;
    f32 time_since_last_pickup = MAX_PICKUP_FLASH_DURATION;
  
    //openGL
    GLuint VAO;
    GLuint VBO;

    const ShaderProgramHandle shader;
};
}