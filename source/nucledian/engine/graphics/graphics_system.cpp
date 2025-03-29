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

namespace nc
{

// Temporary solution
std::vector<ModelHandle> g_map_sector_models;

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

  if (CVars::enable_top_down_debug)
  {
    render_map_top_down();
  }
  else
  {
    m_gizmo_manager.draw_gizmos();
    render_sectors();
    render_entities();
  }

  #ifdef NC_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  #endif

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
void GraphicsSystem::render_map_top_down()
{
  static vec2 input_position   = vec2{0.0f};
  static vec2 pointed_position = vec2{0.0f};
  static f32  frustum_dir      = 0.0f;
  static f32  frustum_deg      = 90.0f;
  static f32  ms_last_frame    = 0.0f;

  static bool follow_mouse      = false;
  static bool inspect_sector    = false;
  static int  inspect_sector_id = 0;
  static bool show_sector_frustums = true;

  if (ImGui::Begin("Test Window"))
  {
    ImGui::SliderFloat("X",           &input_position.x, -1.0f, 1.0f);
    ImGui::SliderFloat("Y",           &input_position.y, -1.0f, 1.0f);
    ImGui::SliderFloat("Frustum Dir", &frustum_dir,         0.0f, 360.0f);
    ImGui::SliderFloat("Frustum Deg", &frustum_deg,         0.0f, 180.0f);
    ImGui::Separator();
    ImGui::Checkbox("Follow mouse",   &follow_mouse);
    ImGui::Checkbox("Inspect sector", &inspect_sector);
    ImGui::InputInt("Inspected sector", &inspect_sector_id, 1);
    ImGui::Checkbox("Show sector frustums", &show_sector_frustums);
    ImGui::Text("MS last frame: %.4f", ms_last_frame);
  }
  ImGui::End();

  auto& map = get_engine().get_map();

  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  int mx = 0, my = 0;
  SDL_GetMouseState(&mx, &my);
  vec2 sz = vec2{(f32)width, (f32)height};

  if (follow_mouse)
  {
    pointed_position = ((vec2{(f32)mx, (f32)my} / sz) - vec2{0.5f}) * vec2{2.0f, -2.0f};
  }
  else
  {
    pointed_position = input_position;
  }

  auto debug_print_text = [&](vec2 coords, cstr text, ImU32 col = 0xFFFFFFFF)
  {
    coords = (coords * vec2{1.0f, -1.0f} + vec2{1}) * vec2{0.5f} * vec2{(f32)width, (f32)height};
    ImGui::GetForegroundDrawList()->AddText(ImVec2{coords.x, coords.y}, col, text);
  };

  s32 point_sector = -1;
  //auto from_map = map.get_sector_from_point(pointed_position);
  //if (from_map != INVALID_SECTOR_ID)
  //{
  //  point_sector = static_cast<s32>(from_map);
  //}
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

  auto view_direction = vec2{std::cosf(deg2rad(frustum_dir)), std::sinf(deg2rad(frustum_dir))};

  std::map<SectorID, u32>                   visible_sectors;
  std::map<SectorID, std::vector<Frustum2>> visible_sectors_frustum;
  auto start_time = std::chrono::high_resolution_clock::now();

  std::vector<Frustum2> inspected_sector_frustums;

  map.query_visible_sectors(pointed_position, view_direction, deg2rad(frustum_deg), [&](SectorID id, Frustum2 f, PortalID)
  {
    if (!visible_sectors.contains(id))
    {
      visible_sectors[id] = 0;
    }

    visible_sectors_frustum[id].push_back(f);
    visible_sectors[id] += 1;

    if (inspect_sector && inspect_sector_id == id)
    {
      inspected_sector_frustums.push_back(f);
    }
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
    [[maybe_unused]]const auto vis_count  = visible_sectors.contains(i) ? visible_sectors[i] : 0;

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

      vec3 col;
      if (show_sector_frustums)
      {
        col = vec3{0.0f};
      }
      else
      {
        col = pointed_at ? vec3{0.75f} : vec3{vis_count * 0.1f};
      }

      this->get_gizmo_manager().draw_triangle(
        first_wall.pos,
        wall1.pos,
        wall2.pos,
        col
      );
    }
  }

  // Render frustums for all visible sectors
  if (show_sector_frustums)
  {
    glEnable(GL_STENCIL_TEST);

    for (const auto&[sector_id, frustums] : visible_sectors_frustum)
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
          this->get_gizmo_manager().draw_triangle(
            first_wall.pos,
            wall1.pos,
            wall2.pos,
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
        for (const auto& frustum : frustums)
        {
          vec2 le, re;
          frustum.get_frustum_edges(le, re);

          const auto color = vec3{0.25f, 0.25f, 0.25f};
          this->get_gizmo_manager().draw_triangle(
            frustum.center,
            frustum.center + le * 2.0f,
            frustum.center + re * 2.0f,
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
      this->get_gizmo_manager().draw_line(
        wall1.pos,
        wall2.pos,
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
      this->get_gizmo_manager().draw_line(
        wall1.pos,
        wall2.pos,
        vec3{1, 0, 0}
      );
    }
  }

  // Horizontal
  this->get_gizmo_manager().draw_line(
    vec2{-1.0f, pointed_position.y},
    vec2{1.0f,  pointed_position.y},
    vec3{1.0f}
  );

  // Vertical
  this->get_gizmo_manager().draw_line(
    vec2{pointed_position.x,  1.0f},
    vec2{pointed_position.x, -1.0f},
    vec3{1.0f}
  );

  // Left one
  const auto ldir  = deg2rad(frustum_dir + frustum_deg * 0.5f);
  const auto rdir  = deg2rad(frustum_dir - frustum_deg * 0.5f);
  const auto ledge = vec2{std::cosf(ldir), std::sinf(ldir)};
  const auto redge = vec2{std::cosf(rdir), std::sinf(rdir)};

  this->get_gizmo_manager().draw_line(
    pointed_position,
    pointed_position + ledge * 2.0f,
    vec3{0.5f, 0.5f, 1.0f}
  );

  this->get_gizmo_manager().draw_line(
    pointed_position,
    pointed_position + redge * 2.0f,
    vec3{0.5f, 0.5f, 1.0f}
  );

  if (inspect_sector)
  {
    // render the frustums of the inspected sector
    int fidx = 0;
    constexpr int COL_CNT = 3;
    constexpr auto COLS = std::array{vec3{0.75f, 0.2f, 0.2f}, vec3{0.2f, 0.75f, 0.2f}, vec3{0.2f, 0.2f, 0.75f}};
    for (auto& frustum : inspected_sector_frustums)
    {
      vec2 le, re;
      frustum.get_frustum_edges(le, re);

      auto color = COLS[(fidx++) % COL_CNT];

      // red cross
      this->get_gizmo_manager().draw_line(
        frustum.center + vec2{-1, 0} * 0.1f,
        frustum.center + vec2{ 1, 0} * 0.1f,
        vec3{1.0f, 0.5f, 0.5f}
      );

      this->get_gizmo_manager().draw_line(
        frustum.center + vec2{0,  1} * 0.1f,
        frustum.center + vec2{0, -1} * 0.1f,
        vec3{1.0f, 0.5f, 0.5f}
      );

      this->get_gizmo_manager().draw_triangle(
        frustum.center,
        frustum.center + le * 2.0f,
        frustum.center + re * 2.0f,
        color
      );
    }
  }
}

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
void GraphicsSystem::render_sectors() const
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

  const auto& map = get_engine().get_map();
  const auto  pos = cam->get_position();
  const auto  dir = cam->get_forward();
  const auto  fov = std::numbers::pi_v<f32> * 0.5f; // 90 degrees

  const auto pos2   = vec2{pos.x, pos.z};
  const auto dir2   = vec2{dir.x, dir.z};
  const auto dir2_n = is_zero(dir2) ? vec2::X : normalize(dir2);

  std::set<SectorID> sectors_to_render;
  map.query_visible_sectors(pos2, dir2_n, fov, [&](SectorID id, Frustum2, PortalID)
  {
    sectors_to_render.insert(id);
  });

  m_solid_material.use();

  for (auto sector_id : sectors_to_render)
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

