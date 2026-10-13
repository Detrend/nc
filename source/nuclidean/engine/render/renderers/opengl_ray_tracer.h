// Project Nuclidean Source File
#pragma once

#include <math/vector.h>
#include <engine/render/sdl_types.h>
#include <engine/render/opengl/shader.h>
#include <engine/render/renderers/base_renderer.h>

namespace nc::render::renderers
{

class OpenGLRayTracer : public BaseRenderer
{
public:
  bool init() override;
  void terminate() override;

  void on_window_resized(u32 width, u32 height) override;
  void render() override;

private:
  SDL_GLContext m_gl_context = nullptr;

  // Texture the tracing shader renders the frame into.
  GLuint m_output_texture     = 0;
  // Read framebuffer with the output texture attached.
  GLuint m_output_framebuffer = 0;

  // Compute shader tracing the scene into the output texture.
  opengl::Shader m_tracing_shader;
  // Resolution of the output texture.
  uvec2 m_output_resolution = uvec2{0, 0};
};

}
