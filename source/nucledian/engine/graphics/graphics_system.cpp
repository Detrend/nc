// Project Nucledian Source File
#include <engine/graphics/graphics_system.h>

#include <common.h>
#include <config.h>

#include <engine/core/engine.h>
#include <engine/core/module_event.h>
#include <engine/core/engine_module_types.h>

#include <engine/graphics/resources/res_lifetime.h>

#include <engine/entities.h>
#include <engine/input/input_system.h>
#include <engine/map/map_system.h>
#include <engine/player/thing_system.h>

#include <engine/map/map_system.h>
#include <intersect.h>

#include <cvars.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

#include <maths.h>

#include <numbers> // std::numbers::pi
#include <algorithm>
#include <unordered_map>
#include <set>
#include <variant>  // std::visit

// Remove this after logging is added!
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <chrono>

#ifdef NC_DEBUG_DRAW
namespace nc::debug_helpers
{

static Material g_top_down_material = Material::invalid();
static GLuint   g_default_vao       = 0;

//==============================================================================
constexpr cstr TOP_DOWN_FRAGMENT_SOURCE = R"ABC(
#version 430 core
out vec4 FragColor;

layout(location = 0) uniform vec3 u_color;

void main()
{
  FragColor = vec4(u_color, 1.0f);
} 
)ABC";

//==============================================================================
constexpr cstr TOP_DOWN_VERTEX_SOURCE = R"ABC(
#version 430 core
layout (location = 0) in vec3 aPos;

void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)ABC";
  
//==============================================================================
static void draw_line(
  const vec2& from, const vec2& to, const vec3& color = vec3{1})
{
  g_top_down_material.use();

  const auto  screen_bbox = aabb2{vec2{-1}, vec2{1}};
  const aabb2 bbox = aabb2{from, to};

  if (!intersect::aabb_aabb_2d(bbox, screen_bbox))
  {
    return;
  }

  glBindVertexArray(g_default_vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[2] = {vec3(from, -0.5f), vec3(to, -0.5f)};
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 2, data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, color.x, color.y, color.z);

  glDrawArrays(GL_LINES, 0, 2);

  glDeleteBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//==============================================================================
static void draw_triangle(
  const vec2& a, const vec2& b, const vec2& c, const vec3& color)
{
  g_top_down_material.use();

  const auto  screen_bbox = aabb2{vec2{-1}, vec2{1}};
  const aabb2 bbox = aabb2{a, b, c};

  if (!intersect::aabb_aabb_2d(bbox, screen_bbox))
  {
    return;
  }

  glBindVertexArray(g_default_vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[3] = 
  {
    vec3(a, -0.5f),
    vec3(b, -0.5f),
    vec3(c, -0.5f),
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, color.x, color.y, color.z);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDeleteBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//==============================================================================
[[maybe_unused]]static void draw_text(vec2 coords, cstr text, vec3 color)
{
  const f32 width  = ImGui::GetWindowWidth();
  const f32 height = ImGui::GetWindowHeight();

  const auto col = ImColor{color.x, color.y, color.z, 1.0f};

  coords = (coords * vec2{1.0f, -1.0f} + vec2{1}) * vec2{0.5f} * vec2{width, height};
  ImGui::GetForegroundDrawList()->AddText(ImVec2{coords.x, coords.y}, col, text);
};

}
#endif

namespace nc
{

// MR says: keeping it here instead of the header so we do not need to include
// intersect.h and map_types.h to graphics_system.h
struct VisibleSectors
{
  std::map<SectorID, FrustumBuffer> sectors;
  vec2 position;
  vec2 direction;
  f32  fov;
};

// Temporary solution
std::vector<ModelHandle> g_map_sector_models;

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
  s32 numeric_severity = 0;
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_NOTIFICATION: numeric_severity = 0; break;
    case GL_DEBUG_SEVERITY_LOW:          numeric_severity = 1; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       numeric_severity = 2; break;
    case GL_DEBUG_SEVERITY_HIGH:         numeric_severity = 3; break;
    default: NC_ERROR("Unknown severity");                     break;
  }

  if (numeric_severity < CVars::opengl_debug_severity)
  {
    return;
  }

  NC_TODO("We need a proper logging system!!!");
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
    [[maybe_unused]] cstr error = SDL_GetError();
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,          1);
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

  #ifdef NC_DEBUG_DRAW
  debug_helpers::g_top_down_material = Material
  (
    debug_helpers::TOP_DOWN_VERTEX_SOURCE,
    debug_helpers::TOP_DOWN_FRAGMENT_SOURCE
  );
  glGenVertexArrays(1, &debug_helpers::g_default_vao);
  #endif

  // init imgui
  #ifdef NC_IMGUI
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);
  #endif

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

    case ModuleEventType::post_init:
    {
      build_map_gfx();
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
const DebugCamera& GraphicsSystem::get_debug_camera() const
{
  return m_debug_camera;
}

//==============================================================================
void GraphicsSystem::terminate()
{
  #ifdef NC_IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  #endif

  m_model_manager.unload<ResLifetime::Game>();
  m_mesh_manager.unload<ResLifetime::Game>();

  SDL_GL_DeleteContext(m_gl_context);
  m_gl_context = nullptr;

  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}

//==============================================================================
static void query_data_from_camera(
  const DebugCamera& camera,
  vec2& pos,
  vec2& dir,
  f32&  fov)
{
  pos = vec2{camera.get_position().x, camera.get_position().z};

  vec2 d = vec2{camera.get_forward().x, camera.get_forward().z};
  dir = is_zero(d) ? vec2{1, 0} : normalize(d);

  fov = std::numbers::pi_v<f32> * 0.5f; // 90 degrees
}

//==============================================================================
void GraphicsSystem::query_visible_sectors(VisibleSectors& out) const
{
  NC_ASSERT(out.sectors.empty());

  const auto& map = get_engine().get_map();

  auto* camera = this->get_camera();
  if (!camera)
  {
    return;
  }

  query_data_from_camera(*camera, out.position, out.direction, out.fov);

  NC_TODO("This does not work properly, because we can visit one sector multiple times");
  map.query_visible_sectors(out.position, out.direction, out.fov,
  [&](SectorID sid, Frustum2 frst, PortalID)
  {
    out.sectors[sid].insert_frustum(frst);
  });
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
  //m_debug_camera.handle_input(delta_seconds);

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

  #ifdef NC_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  #endif

  #ifdef NC_DEBUG_DRAW
  if (CVars::display_debug_window)
  {
    draw_debug_window();
  }
  #endif

  VisibleSectors visible;
  query_visible_sectors(visible);

  if (CVars::enable_top_down_debug)
  {
    // Top down rendering for easier debugging
    #ifdef NC_DEBUG_DRAW
    render_map_top_down(visible);
    #endif
  }
  else
  {
    m_gizmo_manager.draw_gizmos();
    render_sectors(visible);
    render_entities(visible);
  }

  #ifdef NC_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  #endif

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
#ifdef NC_DEBUG_DRAW
void GraphicsSystem::render_map_top_down(const VisibleSectors& visible_sectors)
{
  glClear(GL_STENCIL_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  static vec2 input_position   = vec2{0.0f};
  static vec2 pointed_position = vec2{0.0f};
  static vec2 player_direction = vec2{1, 0};
  static f32  frustum_dir      = 0.0f;
  static f32  frustum_deg      = 90.0f;

  static bool follow_mouse      = false;
  static bool inspect_sector    = false;
  static int  inspect_sector_id = 0;
  static bool show_sector_frustums = true;

  static f32 zoom = 0.1f;

  if (auto* camera = this->get_camera())
  {
    pointed_position = vec2{camera->get_position().x, camera->get_position().z};
    const auto frwd  = vec2{camera->get_forward().x,  camera->get_forward().z};
    player_direction = is_zero(frwd) ? vec2{1, 0} : normalize(frwd);
  }

  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);
  const f32 aspect = (f32)width / height;

  auto level_space_to_screen_space = [&](vec2 pos) -> vec2
  {
    return vec2{1.0f, aspect} * (pos - pointed_position) * zoom;
  };

  auto draw_player = [&](vec2 coords, vec2 dir, vec3 color1, vec3 color2, f32 scale)
  {
    const vec2 front = level_space_to_screen_space(coords + dir * scale);
    const vec2 back  = level_space_to_screen_space(coords - dir * 0.5f * scale);
    const vec2 left  = level_space_to_screen_space(coords + flipped(dir) * scale * 0.5f - dir * scale);
    const vec2 right = level_space_to_screen_space(coords - flipped(dir) * scale * 0.5f - dir * scale);

    debug_helpers::draw_triangle(front, back, left,  color1);
    debug_helpers::draw_triangle(front, back, right, color1);

    debug_helpers::draw_line(front, left,  color2);
    debug_helpers::draw_line(left,  back,  color2);
    debug_helpers::draw_line(back,  right, color2);
    debug_helpers::draw_line(right, front, color2);
  };

  if (ImGui::Begin("Test Window"))
  {
    ImGui::SliderFloat("X",           &input_position.x, -1.0f,  1.0f);
    ImGui::SliderFloat("Y",           &input_position.y, -1.0f,  1.0f);
    ImGui::SliderFloat("Frustum Dir", &frustum_dir,       0.0f,  360.0f);
    ImGui::SliderFloat("Frustum Deg", &frustum_deg,       0.0f,  180.0f);
    ImGui::SliderFloat("Zoom",        &zoom,              0.01f, 1.0f);

    ImGui::Separator();

    ImGui::Checkbox("Follow mouse",   &follow_mouse);
    ImGui::Checkbox("Inspect sector", &inspect_sector);
    ImGui::InputInt("Inspected sector", &inspect_sector_id, 1);
    ImGui::Checkbox("Show sector frustums", &show_sector_frustums);
  }
  ImGui::End();

  auto& map = get_engine().get_map();

  auto view_direction = vec2{std::cosf(deg2rad(frustum_dir)), std::sinf(deg2rad(frustum_dir))};

  // first, render the floors of the sectors with gray
  for (SectorID i = 0; i < map.sectors.size(); ++i)
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
    const bool  is_visible = visible_sectors.sectors.contains(i);

    const auto color = is_visible ? colors::GRAY : colors::BLACK;

    for (WallID index = 1; index < wall_count; ++index)
    {
      WallID next_index = (index+1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr  = repr.first_wall + next_index;
      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      debug_helpers::draw_triangle(
        level_space_to_screen_space(first_wall.pos),
        level_space_to_screen_space(wall1.pos),
        level_space_to_screen_space(wall2.pos),
        color
      );
    }
  }

  // Render frustums for all visible sectors
  if (show_sector_frustums)
  {
    glEnable(GL_STENCIL_TEST);

    for (const auto&[sector_id, frustums] : visible_sectors.sectors)
    {
      glStencilMask(0xFF);
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glColorMask(false, false, false, false); // do not write into the color buffer

      glClearStencil(0);
      glClear(GL_STENCIL_BUFFER_BIT);

      // render the sector shape into stencil buffer
      {
        const auto& sector = map.sectors[sector_id];
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
          debug_helpers::draw_triangle(
            level_space_to_screen_space(first_wall.pos),
            level_space_to_screen_space(wall1.pos),
            level_space_to_screen_space(wall2.pos),
            vec3{1}
          );
        }
      }

      // render the frustum only on parts where the stencil buffer is enabled
      {
        glColorMask(true, true, true, true);    // turn on the color back again
        glStencilFunc(GL_EQUAL, 1, 0xFF);       // fail the test if the value is not 1
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // keep the value in stencil buffer even if we fail the test

        // render the frustum, but with the stencil test, so only the pixels inside the sector pass
        for (const auto& frustum : frustums.frustum_slots)
        {
          if (frustum == INVALID_FRUSTUM)
          {
            continue;
          }

          vec2 le, re;
          frustum.get_frustum_edges(le, re);

          const auto color = vec3{0.25f, 0.25f, 0.25f};
          debug_helpers::draw_triangle(
            level_space_to_screen_space(frustum.center),
            level_space_to_screen_space(frustum.center + le * 3000.0f),
            level_space_to_screen_space(frustum.center + re * 3000.0f),
            color
          );
        }
      }
    }

    // cleanup the stencil buffer after ourselves
    {
      glClear(GL_STENCIL_BUFFER_BIT);
      glDisable(GL_STENCIL_TEST);
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
      debug_helpers::draw_line(
        level_space_to_screen_space(wall1.pos),
        level_space_to_screen_space(wall2.pos),
        vec3{1}
      );
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
      WallID index_in_arr = map.portals[repr.first_portal+index].wall_index + repr.first_wall;
      WallID next_in_arr  = index_in_arr+1;
      if (next_in_arr >= repr.last_wall)
      {
        next_in_arr = static_cast<WallID>(next_in_arr - wall_count);
      }

      NC_ASSERT(index_in_arr < map.walls.size());
      NC_ASSERT(next_in_arr  < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      debug_helpers::draw_line(
        level_space_to_screen_space(wall1.pos),
        level_space_to_screen_space(wall2.pos),
        vec3{1, 0, 0}
      );
    }
  }

  draw_player
  (
    pointed_position,
    player_direction,
    colors::GRAY,
    colors::ORANGE,
    0.5f
  );

  // Left one
  const auto ldir  = deg2rad(frustum_dir + frustum_deg * 0.5f);
  const auto rdir  = deg2rad(frustum_dir - frustum_deg * 0.5f);
  const auto ledge = vec2{std::cosf(ldir), std::sinf(ldir)};
  const auto redge = vec2{std::cosf(rdir), std::sinf(rdir)};

  debug_helpers::draw_line(
    level_space_to_screen_space(pointed_position),
    level_space_to_screen_space(pointed_position + ledge * 2.0f),
    vec3{0.5f, 0.5f, 1.0f}
  );

  debug_helpers::draw_line(
    level_space_to_screen_space(pointed_position),
    level_space_to_screen_space(pointed_position + redge * 2.0f),
    vec3{0.5f, 0.5f, 1.0f}
  );

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}
#endif

//==============================================================================
#ifdef NC_DEBUG_DRAW
static void draw_cvar_type_and_input(f32* flt, const CVars::CVarRange& range)
{
  ImGui::TableNextColumn();
  ImGui::Text("f32");

  ImGui::TableNextColumn();
  ImGui::PushID(flt);
  ImGui::SliderFloat("", flt, range.min, range.max);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(s32* num, const CVars::CVarRange& rn)
{
  ImGui::TableNextColumn();
  ImGui::Text("s32");

  ImGui::TableNextColumn();
  ImGui::PushID(num);
  ImGui::SliderInt("", num, static_cast<s32>(rn.min), static_cast<s32>(rn.max));
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(std::string* str, const CVars::CVarRange&)
{
  ImGui::TableNextColumn();
  ImGui::Text("string");

  ImGui::TableNextColumn();
  ImGui::PushID(str);
  ImGui::InputText("", str);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(bool* bl, const CVars::CVarRange&)
{
  ImGui::TableNextColumn();
  ImGui::Text("bool");

  ImGui::TableNextColumn();
  ImGui::PushID(bl);
  ImGui::Checkbox("", bl);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_row(const std::string& name, const CVars::CVar& cvar)
{
  #ifdef NC_COMPILER_CLANG
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wformat-security"
  #endif

  ImGui::TableNextColumn();
  ImGui::Text(name.c_str());
  ImGui::SetItemTooltip(cvar.desc);

  #ifdef NC_COMPILER_CLANG
  #pragma clang diagnostic pop
  #endif

  auto& range_list = CVars::get_cvar_ranges();
  auto  it         = range_list.find(name);

  const auto& range = it != range_list.end() ? it->second : CVars::DEFAULT_RANGE;

  std::visit([&](auto&& t){ draw_cvar_type_and_input(t, range); }, cvar.ptr);
}

//==============================================================================
void GraphicsSystem::draw_debug_window()
{
  if (CVars::display_imgui_demo)
  {
    ImGui::ShowDemoWindow(&CVars::display_imgui_demo);
  }

  if (ImGui::Begin("Debug window", &CVars::display_debug_window))
  {
    if (ImGui::BeginTabBar("Tab Bar"))
    {
      if (ImGui::BeginTabItem("CVars"))
      {
        auto& cvar_list = CVars::get_cvar_list();

        if (ImGui::BeginTable("cvar list", 3, ImGuiTableFlags_Borders))
        {
          ImGui::TableSetupColumn("Name");
          ImGui::TableSetupColumn("Type");
          ImGui::TableSetupColumn("Value");
          ImGui::TableHeadersRow();

          for (const auto&[name, cvar] : cvar_list)
          {
            ImGui::TableNextRow();
            draw_cvar_row(name, cvar);
          }

          ImGui::EndTable();
        }

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Other"))
      {
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}
#endif

//==============================================================================
void GraphicsSystem::render_sectors(const VisibleSectors& visible) const
{
  constexpr auto SECTOR_COLORS = std::array
  {
    colors::YELLOW ,
    colors::CYAN   ,
    colors::MAGENTA,
    colors::ORANGE ,
    colors::PURPLE ,
    colors::PINK   ,
    colors::GRAY   ,
    colors::BROWN  ,
    colors::LIME   ,
    colors::TEAL   ,
    colors::NAVY   ,
    colors::MAROON ,
    colors::OLIVE  ,
    colors::SILVER ,
    colors::GOLD   ,
  };

  auto* cam = this->get_camera();
  if (!cam)
  {
    return;
  }

  const auto& sectors_to_render = visible.sectors;

  m_solid_material.use();

  for (const auto&[sector_id, _] : sectors_to_render)
  {
    NC_ASSERT(sector_id < g_map_sector_models.size());
    auto handle = g_map_sector_models[sector_id];

    glBindVertexArray(handle->mesh.get_vao());

    m_solid_material.set_uniform(shaders::solid::VIEW,          cam->get_view());
    m_solid_material.set_uniform(shaders::solid::VIEW_POSITION, cam->get_position());

    const auto transform = mat4{1.0f};
    const auto col       = SECTOR_COLORS[sector_id % SECTOR_COLORS.size()];
    m_solid_material.set_uniform(shaders::solid::COLOR,     col);
    m_solid_material.set_uniform(shaders::solid::TRANSFORM, transform);

    glDrawArrays(handle->mesh.get_draw_mode(), 0, handle->mesh.get_vertex_count());
  }

  glBindVertexArray(0);
}

//==============================================================================
void GraphicsSystem::render_entities(const VisibleSectors&) const
{
  /*
   * 1. obtain visible secors from sector system (TODO)
   * 2. get RenderComponents from entities within visible sectors (TODO)
   * 3. filer visible entities (TODO)
   * 4. group by ModelHandle
   * 5. sort groups by: 1. program, 2. texture, 3. vao (TODO)
   * 5. issue render command for each group (TODO)
   */

  auto cam = this->get_camera();
  if (!cam)
  {
    return;
  }

  // Group entities by Models
  // MR says: For some stupid reason using unordered_map<ModelHandle, ...> does
  // not work properly (even though the specialized std::hash seems to be ok).
  // Using ModelHandle causes some elements to end up colliding with each and being
  // put into the same bucket on the same index (they are effectively being treated
  // by the hash map as being the same, even though they have different values
  // and a different hash).
  // I did not have time to spend more time on this this so therefore I introduced
  // u32 and ModelGroup - these seem to work exactly as expected.
  struct ModelGroup
  {
    ModelHandle      handle;
    std::vector<u32> component_indices;
  };
  std::unordered_map<u64, ModelGroup> model_groups;
  for (u32 i = 0; i < g_appearance_components.size(); ++i)
  {
    const auto handle = g_appearance_components[i].model_handle;
    model_groups[handle.m_model_id].handle = handle;
    model_groups[handle.m_model_id].component_indices.push_back(i);
  }

  // TODO: sort groups by: 1. program, 2. texture, 3. vao
  for (const auto& [_, group] : model_groups)
  {
    const auto&[handle, indices] = group;

    // TODO: switch program & vao only when neccesary
    handle->material.use();
    glBindVertexArray(handle->mesh.get_vao());

    // TODO: some uniform locations should be shader independent
    handle->material.set_uniform(shaders::solid::VIEW,          cam->get_view());
    handle->material.set_uniform(shaders::solid::VIEW_POSITION, cam->get_position());

    // TODO: indirect rendering
    for (auto& index : indices)
    {
      const Position& position = m_position_components[index];
      const Appearance& appearance = g_appearance_components[index];

      const mat4 transform = scale(rotate(translate(mat4(1.0f), position), appearance.rotation, vec3::Y), appearance.scale);
      // TODO: should be set only when these changes and probably not here
      handle->material.set_uniform(shaders::solid::COLOR, appearance.model_color);
      handle->material.set_uniform(shaders::solid::TRANSFORM, transform);

      glDrawArrays(handle->mesh.get_draw_mode(), 0, handle->mesh.get_vertex_count());
    }
  }
}

//==============================================================================
DebugCamera* GraphicsSystem::get_camera() const
{
  auto& things = get_engine().get_module<ThingSystem>();
  return things.get_player()->get_camera();
}

//==============================================================================
void GraphicsSystem::build_map_gfx()
{
  const auto& m_map = get_engine().get_map();

  auto& mesh_man        = get_mesh_manager();
  auto& model_man       = get_model_manager();
  const auto& solid_mat = get_solid_material();

  for (SectorID sid = 0; sid < m_map.sectors.size(); ++sid)
  {
    std::vector<vec3> vertices;
    m_map.sector_to_vertices(sid, vertices);

    const f32* vertex_data = &vertices[0].x;
    const u32  values_cnt  = static_cast<u32>(vertices.size() * 3);

    const auto mesh   = mesh_man.create<ResLifetime::Game>(vertex_data, values_cnt);
    const auto handle = model_man.add<ResLifetime::Game>(mesh, solid_mat);

    g_map_sector_models.push_back(handle);
  }
}

}

