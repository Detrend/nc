// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

#include <vec.h>

#include <vector>

struct SDL_Window;

namespace nc
{

struct ModuleEvent;

class GraphicsSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  bool init();
  void on_event(ModuleEvent& event) override;

  void update_window_and_pump_messages();

  void push_line(const vec2& from, const vec2& to, const vec3& col = vec3{1});
  void push_triangle(
    const vec2& a,
    const vec2& b,
    const vec2& c,
    const vec3& col);

private:
  void terminate();
  void render();
  void render_map();
  void compile_the_retarded_shaders();

  enum PrimitiveType : u8
  {
    LineType,
    TriangleType,
    // - //
    Count,
  };

  struct Line
  {
    vec2 from;
    vec2 to;
    vec3 color;
  };

  struct Triangle
  {
    vec2 a;
    vec2 b;
    vec2 c;
    vec3 color;
  };

  struct Primitive
  {
    PrimitiveType type;
    union
    {
      Line     line;
      Triangle triangle;
    };
  };

  void render_line(const Line& line);
  void render_triangle(const Triangle& tri);

private:
  SDL_Window* m_window     = nullptr;
  void*       m_gl_context = nullptr;
  u32         m_vao        = 0;

  std::vector<Primitive> m_primitives;

  u32 m_funckin_gpu_program;
};

}