// Project Nuclidean Source File
#pragma once
#include <engine/graphics/resources/shader_program.h>

namespace nc 
{
class UiHudDisplay
{
public:
  UiHudDisplay();
  ~UiHudDisplay();

  void update(float delta_time);

  void show_secret();

  void draw();
    
  void set_crosshair(int val);

private:
  void init();

  void draw_digits();
  void draw_health();
  void draw_ammo();
  void draw_texts();
  void draw_crosshair();
  void draw_secret_revealed();

  int display_ammo = 0;
  int display_health = 0;
  int crosshair = 1;

  float time_since_secret = TIME_TO_SHOW_SECRET;

  const float TIME_TO_SHOW_SECRET = 3.0f;

  GLuint VAO;
  GLuint VBO;

  const ShaderProgramHandle digit_shader;
  const ShaderProgramHandle text_shader;
};
}