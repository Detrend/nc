// Project Nucledian Source File
#include <engine/graphics/graphics_system.h>

#include <common.h>
#include <engine/core/engine.h>
#include <engine/core/module_event.h>
#include <engine/core/engine_module_types.h>
#include <engine/input/input_system.h>
#include <engine/graphics/resources/res_lifetime.h>
#include <engine/entities.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#include <iostream>
#include <algorithm>
#include <ranges>
#include <unordered_map>

namespace nc
{

//==============================================================================
EngineModuleId GraphicsSystem::get_module_id()
{
  return EngineModule::graphics_system;
}

//==============================================================================
static void APIENTRY gl_debug_message(
  GLenum /*source*/, GLenum /*type*/, GLuint /*id*/, GLenum /*severity*/,
  GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
{
  std::cout << "GL Debug Message: " << message << std::endl;
}

//==============================================================================
bool GraphicsSystem::init()
{
  NC_TODO("Log out an error when the graphics system initialization fails");
  NC_TODO("Terminate the already set-up SDL stuff on failed initialization.");

  // init SDL
  constexpr auto SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
  constexpr u32  SDL_WIN_FLAGS  = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  constexpr cstr WINDOW_NAME    = "Nucledian";
  constexpr auto WIN_POS        = SDL_WINDOWPOS_UNDEFINED;

  if (SDL_Init(SDL_INIT_FLAGS) < 0)
  {
    // failed to init SDL, see what's the issue
    [[maybe_unused]]cstr error = SDL_GetError();
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  // create window
  m_window = SDL_CreateWindow(
    WINDOW_NAME, WIN_POS, WIN_POS, 800, 600, SDL_WIN_FLAGS);
  if (!m_window)
  {
    [[maybe_unused]]cstr error = SDL_GetError();
    return false;
  }

  // create opengl context
  m_gl_context = SDL_GL_CreateContext(m_window);
  if (!m_gl_context)
  {
    [[maybe_unused]]cstr error = SDL_GetError();
    return false;
  }

  // init opengl bindings
  if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
  {
    return false;
  }

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(&gl_debug_message, nullptr);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_MULTISAMPLE);

  m_mesh_manager.init();

  m_solid_material = Material(shaders::solid::VERTEX_SOURCE, shaders::solid::FRAGMENT_SOURCE);
  m_cube_model_handle = m_model_manager.add<ResLifetime::Game>(m_mesh_manager.get_cube(), m_solid_material);

  const mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, projection);

  return true;
}

//==============================================================================
void GraphicsSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::game_update:
    {
      this->update(event.update.dt);
      break;
    }

    case ModuleEventType::render:
    {
      this->render();
      break;
    }

    case ModuleEventType::terminate:
    {
      this->terminate();
      break;
    }
  }
}

//==============================================================================
void GraphicsSystem::update_window_and_pump_messages()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_QUIT:
      {
        get_engine().request_quit();
        break;
      }

      case SDL_KEYDOWN:
      {
        // TODO: move to input system
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          get_engine().request_quit();
        }
      }
      case SDL_KEYUP:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEWHEEL:
      case SDL_MOUSEMOTION:
      {
        InputSystem::get().handle_app_event(event);
        break;
      }
    }
  }
}

//==============================================================================
const DebugCamera& GraphicsSystem::get_debug_camera() const
{
  return m_debug_camera;
}

//==============================================================================
MeshManager& GraphicsSystem::get_mesh_manager()
{
  return m_mesh_manager;
}

//==============================================================================
ModelManager& GraphicsSystem::get_model_manager()
{
  return m_model_manager;
}

//==============================================================================
GizmoManager& GraphicsSystem::get_gizmo_manager()
{
  return m_gizmo_manager;
}

//==============================================================================
const Material& GraphicsSystem::get_solid_material() const
{
  return m_solid_material;
}

//==============================================================================
ModelHandle GraphicsSystem::get_cube_model_handle() const
{
  return m_cube_model_handle;
}

//==============================================================================
void GraphicsSystem::update(f32 delta_seconds)
{
  // TODO: only temporary for debug camera
  m_debug_camera.handle_input(delta_seconds);
  SDL_WarpMouseInWindow(m_window, 400, 300);
  int x, y;
  SDL_GetRelativeMouseState(&x, &y);

  m_gizmo_manager.update_ttls(delta_seconds);
}

//==============================================================================
void GraphicsSystem::render()
{
  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_gizmo_manager.draw_gizmos();
  render_entities();
  
  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
void GraphicsSystem::terminate()
{
  SDL_GL_DeleteContext(m_gl_context);
  m_gl_context = nullptr;

  SDL_DestroyWindow(m_window);
  m_window = nullptr;

  m_model_manager.unload<ResLifetime::Game>();
  m_mesh_manager.unload<ResLifetime::Game>();
}

//==============================================================================
void GraphicsSystem::render_entities() const
{
  /*
   * 1. obtain visible secors from sector system (TODO)
   * 2. get RenderComponents from entities within visible sectors (TODO)
   * 3. filer visible entities (TODO)
   * 4. group by ModelHandle
   * 5. sort groups by: 1. program, 2. texture, 3. vao (TODO)
   * 5. issue render command for each group (TODO)
   */

  // group entities by Models
  std::unordered_map<ModelHandle, std::vector<u32>> model_groups;
  for (u32 i = 0; i < g_appearance_components.size(); ++i)
  {
    model_groups[g_appearance_components[i].model_handle].push_back(i);
  }

  // TODO: sort groups by: 1. program, 2. texture, 3. vao

  for (const auto& [handle, indices] : model_groups)
  {
    // TODO: switch program & vao only when neccesary
    handle->material.use();
    glBindVertexArray(handle->mesh.get_vao());

    // TODO: some uniform locations should be shader independent
    handle->material.set_uniform(shaders::solid::VIEW, m_debug_camera.get_view());
    handle->material.set_uniform(shaders::solid::VIEW_POSITION, m_debug_camera.get_position());

    // TODO: indirect rendering
    for (auto& index : indices)
    {
      const Position& position = m_position_components[index];
      const Appearance& appearance = g_appearance_components[index];

      const mat4 transform = scale(translate(mat4(1.0f), position + appearance.y_offset * vec3::Z), appearance.scale);
      // TODO: should be set only when these changes and probably not here
      handle->material.set_uniform(shaders::solid::COLOR, appearance.model_color);
      handle->material.set_uniform(shaders::solid::TRANSFORM, transform);

      glDrawArrays(handle->mesh.get_draw_mode(), 0, handle->mesh.get_vertex_count());
    }
  }
}

}

