// Project Nucledian Source File
#include <common.h>
#include <types.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>

#include <engine/map/map_system.h>
#include <intersect.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

#include <maths.h>

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <chrono>

namespace nc
{

constexpr cstr FRAGMENT_FUCKER = R"ABC(
#version 430 core
out vec4 FragColor;

layout(location = 0) uniform vec3 u_color;

void main()
{
  FragColor = vec4(u_color, 1.0f);
} 
)ABC";

constexpr cstr VERTEX_FUCKER = R"ABC(
#version 430 core
layout (location = 0) in vec3 aPos;

void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)ABC";

//==============================================================================
EngineModuleId GraphicsSystem::get_module_id()
{
  return EngineModule::graphics_system;
}

//==============================================================================
static void APIENTRY gl_debug_message(
  GLenum /*source*/, GLenum /*type*/, GLuint /*id*/, GLenum severity,
  GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
{
  if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    return;
  }

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

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(&gl_debug_message, nullptr);

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  this->compile_the_retarded_shaders();

  // init imgui
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);

  return true;
}

//==============================================================================
void GraphicsSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
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
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type)
    {
      case SDL_QUIT:
      {
        get_engine().request_quit();
        break;
      }

      case SDL_KEYDOWN:
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
void GraphicsSystem::push_line(
  const vec2& from,
  const vec2& to,
  const vec3& col)
{
  m_primitives.push_back(Primitive
  {
  .type = PrimitiveType::LineType,
  .line = Line
  {
    .from  = from,
    .to    = to,
    .color = col,
  }});
}

//==============================================================================
void GraphicsSystem::push_triangle(const vec2& a, const vec2& b, const vec2& c, const vec3& col)
{
  m_primitives.push_back(Primitive
  {
    .type = PrimitiveType::TriangleType,
    .triangle = Triangle
    {
      .a = a,
      .b = b,
      .c = c,
      .color = col
    }
  });
}

//==============================================================================
void GraphicsSystem::terminate()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(m_gl_context);
  m_gl_context = nullptr;

  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}

//==============================================================================
void GraphicsSystem::render()
{
  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  this->render_map();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
void GraphicsSystem::render_map()
{
  static vec2 pointed_position = vec2{0.0f};
  static f32  frustum_dir      = 0.0f;
  static f32  frustum_deg      = 90.0f;
  static f32  ms_last_frame    = 0.0f;

  if (ImGui::Begin("Test Window"))
  {
    ImGui::SliderFloat("X",           &pointed_position.x, -1.0f, 1.0f);
    ImGui::SliderFloat("Y",           &pointed_position.y, -1.0f, 1.0f);
    ImGui::SliderFloat("Frustum Dir", &frustum_dir,         0.0f, 360.0f);
    ImGui::SliderFloat("Frustum Deg", &frustum_deg,         0.0f, 180.0f);
    ImGui::Text("MS last frame: %.4f", ms_last_frame);
  }
  ImGui::End();

  glUseProgram(m_funckin_gpu_program);

  auto& map = get_engine().get_map();

  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  vec2 sz = vec2{(f32)width, (f32)height};

  auto debug_print_text = [&](vec2 coords, cstr text, ImU32 col = 0xFFFFFFFF)
  {
    coords = (coords * vec2{1.0f, -1.0f} + vec2{1}) * vec2{0.5f} * vec2{(f32)width, (f32)height};
    ImGui::GetForegroundDrawList()->AddText(ImVec2{coords.x, coords.y}, col, text);
  };

  s32 point_sector = -1;
  // find the sector we are pointing at
  for (u16 i = 0; i < map.sectors.size(); ++i)
  {
    auto&& sector = map.sectors[i];
    auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    NC_ASSERT(wall_count >= 0);

    if (wall_count < 3)
    {
      continue;
    }

    const auto& first_wall = map.walls[repr.first_wall];

    for (WallID index = 1; index < wall_count; ++index)
    {
      WallID next_index = (index+1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr  = repr.first_wall + next_index;
      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];
      if (intersect::point_triangle(pointed_position, first_wall.pos, wall2.pos, wall1.pos))
      {
        point_sector = i;
        break;
      }
    }

    if (point_sector >= 0)
    {
      break;
    }
  }

  const auto player_frustum = Frustum2
  {
    .center    = pointed_position,
    .direction = vec2{std::cosf(deg2rad(frustum_dir)), std::sinf(deg2rad(frustum_dir))},
    .angle     = std::cosf(deg2rad(frustum_deg * 0.5f)),
  };

  std::map<SectorID, u32> visible_sectors;
  auto start_time = std::chrono::high_resolution_clock::now();
  map.query_visible_sectors(player_frustum, [&](SectorID id, Frustum2, PortalID)
  {
    if (!visible_sectors.contains(id))
    {
      visible_sectors[id] = 0;
    }

    visible_sectors[id] += 1;
  });
  auto end_time = std::chrono::high_resolution_clock::now();
  auto delta_time = end_time - start_time;
  ms_last_frame = std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count() * 0.001f;

  // first, render the floors of the sectors with gray
  for (u16 i = 0; i < map.sectors.size(); ++i)
  {
    auto&& sector = map.sectors[i];
    auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    NC_ASSERT(wall_count >= 0);

    if (wall_count < 3)
    {
      continue;
    }

    const bool pointed_at = i == point_sector;
    const auto vis_count  = visible_sectors.contains(i) ? visible_sectors[i] : 0;

    const auto& first_wall = map.walls[repr.first_wall];

    if (wall_count > 0)
    {
      vec2 avg_pos = vec2{0};

      for (WallID index = 0; index < wall_count; ++index)
      {
        WallID index_in_arr = repr.first_wall + index;
        const auto& wall1 = map.walls[index_in_arr];
        auto point_name = std::to_string(index_in_arr);

        if (pointed_at)
        {
          debug_print_text(wall1.pos, point_name.c_str());
        }

        avg_pos = avg_pos + wall1.pos;
      }

      avg_pos = avg_pos / vec2{(f32)wall_count};
      auto sector_name = std::to_string(i);
      const auto col = pointed_at ? ImColor(255, 0, 0, 255) : ImColor(128, 32, 32, 255);
      debug_print_text(avg_pos, sector_name.c_str(), col);
    }

    for (WallID index = 1; index < wall_count; ++index)
    {
      WallID next_index = (index+1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr  = repr.first_wall + next_index;
      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];
      this->render_triangle(Triangle
      {
        .a     = first_wall.pos,
        .b     = wall1.pos,
        .c     = wall2.pos,
        .color = pointed_at ? vec3{0.75f} : vec3{vis_count * 0.5f},
      });
    }
  }

  // then render the walls with white
  for (auto&& sector : map.sectors)
  {
    auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    NC_ASSERT(wall_count >= 0);

    for (WallID index = 0; index < wall_count; ++index)
    {
      WallID next_index = (index+1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr  = repr.first_wall + next_index;
      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];
      this->render_line(Line
      {
        .from = wall1.pos,
        .to   = wall2.pos,
        .color = vec3{1},
      });
    }
  }

  // and then render portals with red
  for (auto&& sector : map.sectors)
  {
    auto& repr = sector.int_data;
    const s32 wall_count   = repr.last_wall   - repr.first_wall;
    const s32 portal_count = repr.last_portal - repr.first_portal;
    NC_ASSERT(portal_count >= 0);
    NC_ASSERT(wall_count   >= 0);

    for (WallID index = 0; index < portal_count; ++index)
    {
      WallID index_in_arr = map.portals[repr.first_portal+index].wall_index;
      WallID next_in_arr  = index_in_arr+1;
      if (next_in_arr >= repr.last_wall)
      {
        next_in_arr = static_cast<WallID>(next_in_arr - wall_count);
      }
      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];
      this->render_line(Line
      {
        .from = wall1.pos,
        .to   = wall2.pos,
        .color = vec3{1, 0, 0},
      });
    }
  }

  // Horizontal
  this->render_line(Line
  {
    .from  = vec2{-1.0f, pointed_position.y},
    .to    = vec2{1.0f,  pointed_position.y},
    .color = vec3{1.0f},
  });

  // Vertical
  this->render_line(Line
  {
    .from  = vec2{pointed_position.x,  1.0f},
    .to    = vec2{pointed_position.x, -1.0f},
    .color = vec3{1.0f},
  });

  // Left one
  const auto ldir  = deg2rad(frustum_dir + frustum_deg * 0.5f);
  const auto rdir  = deg2rad(frustum_dir - frustum_deg * 0.5f);
  const auto ledge = vec2{std::cosf(ldir), std::sinf(ldir)};
  const auto redge = vec2{std::cosf(rdir), std::sinf(rdir)};

  this->render_line(Line
  {
    .from = pointed_position,
    .to   = pointed_position + ledge * 2.0f,
    .color = vec3{0.5f, 0.5f, 1.0f},
  });

  this->render_line(Line
  {
    .from = pointed_position,
    .to   = pointed_position + redge * 2.0f,
    .color = vec3{0.5f, 0.5f, 1.0f},
  });
}

//==============================================================================
void GraphicsSystem::compile_the_retarded_shaders()
{
  // compile the fragment
  auto frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 1, &FRAGMENT_FUCKER, nullptr);
  glCompileShader(frag);
  int ok;
  glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    char log[512];
    glGetShaderInfoLog(frag, 512, NULL, log);
    std::cout << log << std::endl;
  }

  // compile the vertex
  auto vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert, 1, &VERTEX_FUCKER, nullptr);
  glCompileShader(vert);
  glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
  if (!ok)
  {
    char log[512];
    glGetShaderInfoLog(vert, 512, NULL, log);
    std::cout << log << std::endl;
  }

  m_funckin_gpu_program = glCreateProgram();
  glAttachShader(m_funckin_gpu_program, frag);
  glAttachShader(m_funckin_gpu_program, vert);
  glLinkProgram(m_funckin_gpu_program);
  glGetProgramiv(m_funckin_gpu_program, GL_LINK_STATUS, &ok);
  if(!ok)
  {
    char log[512];
    glGetProgramInfoLog(m_funckin_gpu_program, 512, NULL, log);
  }

  glDeleteShader(frag);
  glDeleteShader(vert);
}

//==============================================================================
void GraphicsSystem::render_line(const Line& line)
{
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[2] = {vec3(line.from, -0.5f), vec3(line.to, -0.5f)};
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 2, data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, line.color.x, line.color.y, line.color.z);

  glDrawArrays(GL_LINES, 0, 2);

  glDeleteBuffers(1, &vertex_buffer);
}

//==============================================================================
void GraphicsSystem::render_triangle(const Triangle& tri)
{
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[3] = 
  {
    vec3(tri.a, -0.5f),
    vec3(tri.b, -0.5f),
    vec3(tri.c, -0.5f),
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, tri.color.x, tri.color.y, tri.color.z);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDeleteBuffers(1, &vertex_buffer);
}

}

