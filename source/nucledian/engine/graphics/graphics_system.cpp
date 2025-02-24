// Project Nucledian Source File
#include <common.h>
#include <types.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>

#include <temp_math.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#include <iostream>
#include <string_view>

namespace nc
{
constexpr const char* vertex_shader_source = // vertex shader:
R"(
  #version 430 core
  layout (location = 0) in vec3 in_pos;

  uniform mat4 view;
  uniform mat4 projection;

  void main()
  {
    gl_Position = projection * view * vec4(in_pos, 1.0f);
  }
)";

constexpr const char* fragment_shader_source = // fragment shader:
R"(
  #version 430 core
  out vec4 out_color;

  void main()
  {
    out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  }
)";

constexpr float cube_vertices[] = {
  -0.5f, +0.5f, -0.5f, // 0 left-top-back
  +0.5f, +0.5f, -0.5f, // 1 right-top-back
  -0.5f, -0.5f, -0.5f, // 2 left-bottom-back
  +0.5f, -0.5f, -0.5f, // 3 right-bottom-back
  -0.5f, +0.5f, +0.5f, // 4 left-top-front
  +0.5f, +0.5f, +0.5f, // 5 right-top-front
  -0.5f, -0.5f, +0.5f, // 6 left-bottom-front
  +0.5f, -0.5f, +0.5f, // 7 right-bottom-front
};
constexpr u32 cube_indices[] = {
  3, 2, 1, 0, 4, 2, 6, 3, 7, 1, 5, 4, 7, 6
};

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

  // create window
  m_window = SDL_CreateWindow(
    WINDOW_NAME, WIN_POS, WIN_POS, 640, 480, SDL_WIN_FLAGS);
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

  SDL_SetRelativeMouseMode(SDL_TRUE);

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(&gl_debug_message, nullptr);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // compile shaders
  const u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glCompileShader(vertex_shader);

  const u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(fragment_shader);

  m_shader_program = glCreateProgram();
  glAttachShader(m_shader_program, vertex_shader);
  glAttachShader(m_shader_program, fragment_shader);
  glLinkProgram(m_shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // setup projection matrix
  const mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  glUseProgram(m_shader_program);
  glUniformMatrix4fv(glGetUniformLocation(m_shader_program, "projection"), 1, GL_FALSE, value_ptr(projection));
  glUseProgram(0);

  // gen buffers
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);

  // setup vao
  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);

  return true;
}

//==============================================================================
void GraphicsSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::game_update:
    {
      // TODO: only temporary for debug camera
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

void GraphicsSystem::update(f32 delta_seconds)
{
  // TODO: only temporary for debug camera
  m_debug_camera.handle_input(delta_seconds);
  SDL_WarpMouseInWindow(m_window, 400, 300);
}

//==============================================================================
void GraphicsSystem::render()
{
  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(m_shader_program);
  const mat4 view = m_debug_camera.get_view();
  glad_glUniformMatrix4fv(glGetUniformLocation(m_shader_program, "view"), 1, GL_FALSE, value_ptr(view));
  
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
void GraphicsSystem::terminate()
{
  glDeleteProgram(m_shader_program);
  m_shader_program = 0;
  glDeleteVertexArrays(1, &m_vao);
  m_vao = 0;
  glDeleteBuffers(1, &m_vbo);
  m_vbo = 0;
  glDeleteBuffers(1, &m_ebo);
  m_ebo = 0;

  SDL_GL_DeleteContext(m_gl_context);
  m_gl_context = nullptr;

  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}

}

