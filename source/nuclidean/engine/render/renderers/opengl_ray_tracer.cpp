// Project Nuclidean Source File
#include <engine/render/renderers/opengl_ray_tracer.h>

#include <common.h>
#include <config.h>
#include <logging.h>

#include <engine/render/render_system.h>
#include <engine/render/sdl_utils.h>
#include <engine/render/opengl/utils.h>
#include <engine/render/shaders/tracing.shared.h>

#include <shaders/tracing.h>

#include <glad/glad.h>

#if NC_IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#endif

#include <SDL_video.h>

namespace nc::render::renderers
{

//==============================================================================
bool OpenGLRayTracer::init()
{
  SDL_GL_ResetAttributes();
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  RenderSystem& render_system = RenderSystem::get();
  if (!render_system.recreate_window(SDL_WINDOW_OPENGL))
    return false;
  SDL_Window* window = render_system.get_window();

  m_gl_context = SDL_GL_CreateContext(window);
  if (!m_gl_context)
    return sdl_warn("SDL_GL_CreateContext");

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
    return (nc_warn("[render][opengl ray tracer] gladLoadGLLoader failed"), false);

  SDL_GL_SetSwapInterval(0);

#if NC_DEBUG_DRAW
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(&debug::gl_debug_message, nullptr);
#endif

  glGenFramebuffers(1, &m_output_framebuffer);

  m_tracing_shader = opengl::Shader(tracing);

#if NC_IMGUI
  ImGui_ImplSDL2_InitForOpenGL(window, m_gl_context);
  if (!ImGui_ImplOpenGL3_Init(nullptr))
    return (nc_warn("[render][opengl ray tracer] ImGui_ImplOpenGL3_Init failed"), false);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
#endif

  return true;
}

//==============================================================================
void OpenGLRayTracer::terminate()
{
#if NC_IMGUI
  if (ImGui::GetIO().BackendRendererUserData)
    ImGui_ImplOpenGL3_Shutdown();

  if (ImGui::GetIO().BackendPlatformUserData)
    ImGui_ImplSDL2_Shutdown();
#endif

  if (m_gl_context)
  {
    m_tracing_shader = {};
    SDL_GL_DeleteContext(m_gl_context);
    m_gl_context = nullptr;
  }
}

#if NC_HOT_RELOAD
//==============================================================================
void OpenGLRayTracer::reload_shaders()
{
  m_tracing_shader.reload();
}
#endif

//==============================================================================
void OpenGLRayTracer::on_window_resized(u32 width, u32 height)
{
  m_output_resolution = uvec2(width, height);

  glDeleteTextures(1, &m_output_texture);
  glGenTextures(1, &m_output_texture);
  glBindTexture(GL_TEXTURE_2D, m_output_texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, cast<GLsizei>(width), cast<GLsizei>(height));

  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_output_framebuffer);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_output_texture, 0);
}

//==============================================================================
void OpenGLRayTracer::render()
{
  glBindImageTexture(cast<GLuint>(TracingImage::out_color), m_output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  const uvec2 groups = (m_output_resolution + TRACING_GROUP_SIZE - 1u) / TRACING_GROUP_SIZE;
  m_tracing_shader.dispatch(groups.x, groups.y, 1, true);

  const GLint width = cast<GLint>(m_output_resolution.x);
  const GLint height = cast<GLint>(m_output_resolution.y);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_output_framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if NC_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

  SDL_GL_SwapWindow(RenderSystem::get().get_window());

#if NC_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
#endif
}

}
