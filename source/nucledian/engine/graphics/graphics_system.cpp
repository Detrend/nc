// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/graphics/graphics_system.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/gizmo.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <engine/input/input_system.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/player/thing_system.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_stdlib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

// Remove this after logging is added!
#include <iostream>

#ifdef NC_DEBUG_DRAW
namespace nc::debug_helpers
{

static MaterialHandle g_top_down_material = MaterialHandle::invalid();
static GLuint   g_default_vao = 0;

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
  const vec2& from, const vec2& to, const vec3& color = vec3{ 1 })
{
  g_top_down_material.use();

  const auto  screen_bbox = aabb2{ vec2{-1}, vec2{1} };
  const aabb2 bbox = aabb2{ from, to };

  if (!intersect::aabb_aabb_2d(bbox, screen_bbox))
  {
    return;
  }

  glBindVertexArray(g_default_vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[2] = { vec3(from, -0.5f), vec3(to, -0.5f) };
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

  const auto  screen_bbox = aabb2{ vec2{-1}, vec2{1} };
  const aabb2 bbox = aabb2{ a, b, c };

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
static void draw_text(vec2 coords, cstr text, vec3 color)
{
  int x, y;
  SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &x, &y);
  const f32 width = static_cast<f32>(x);
  const f32 height = static_cast<f32>(y);

  const auto col = ImColor{ color.x, color.y, color.z, 1.0f };

  coords = (coords * vec2{ 1.0f, -1.0f } + vec2{ 1 }) * vec2{ 0.5f } *vec2{ width, height };
  ImGui::GetForegroundDrawList()->AddText(ImVec2{ coords.x, coords.y }, col, text);
};

}
#endif

namespace nc
{

// Temporary solution
std::vector<MeshHandle> g_sector_meshes;

//==============================================================================
GraphicsSystem::GraphicsSystem()
  : m_default_projection(perspective(radians(70.0f), 800.0f / 600.0f, 0.0001f, 100.0f)) {}

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
    default: nc_assert(false, "Unknown severity");                     break;
  }

  if (numeric_severity < CVars::opengl_debug_severity)
  {
    return;
  }
    
  nc_log("GL Debug Message: '{0}'", message);
}

//==============================================================================
bool GraphicsSystem::init()
{
  NC_TODO("Log out an error when the graphics system initialization fails");
  NC_TODO("Terminate the already set-up SDL stuff on failed initialization.");

  // init SDL
  constexpr auto SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
  constexpr u32  SDL_WIN_FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  constexpr cstr WINDOW_NAME = "Nucledian";
  constexpr auto WIN_POS = SDL_WINDOWPOS_UNDEFINED;

  if (SDL_Init(SDL_INIT_FLAGS) < 0)
  {
    // failed to init SDL, see what's the issue
    [[maybe_unused]] cstr error = SDL_GetError();
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  // create window
  m_window = SDL_CreateWindow(
    WINDOW_NAME, WIN_POS, WIN_POS, 800, 600, SDL_WIN_FLAGS);
  if (!m_window)
  {
    [[maybe_unused]] cstr error = SDL_GetError();
    return false;
  }

  // create opengl context
  m_gl_context = SDL_GL_CreateContext(m_window);
  if (!m_gl_context)
  {
    [[maybe_unused]] cstr error = SDL_GetError();
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

  glLineWidth(5.0f);

  MeshManager::instance().init();

  m_solid_material = MaterialHandle(shaders::solid::VERTEX_SOURCE, shaders::solid::FRAGMENT_SOURCE);
  m_cube_model = Model(MeshManager::instance().get_cube(), m_solid_material);

  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, m_default_projection);

#ifdef NC_DEBUG_DRAW
  debug_helpers::g_top_down_material = MaterialHandle
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

    case ModuleEventType::on_map_rebuild:
    {
      build_map_gfx();
      break;
    }

    case ModuleEventType::post_init:
    {
      m_gun_model = m_cube_model;
      m_gun_transform = Transform(
        vec3(0.5f, -0.3f, -1.0f),
        vec3(0.2f, 0.2f, 0.5f),
        vec3(10.0f, 0.0f, 0.0f)
      );
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
void GraphicsSystem::terminate()
{
#ifdef NC_IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
#endif

  MeshManager::instance().unload(ResLifetime::Game);

  SDL_GL_DeleteContext(m_gl_context);
  m_gl_context = nullptr;

  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}

//==============================================================================
void GraphicsSystem::query_visibility(VisibilityTree& tree) const
{
  constexpr u8 DEFAULT_RECURSION_DEPTH = 64;

  nc_assert(tree.sectors.empty());

  const auto& map = get_engine().get_map();

  auto* camera = this->get_camera();
  if (!camera)
  {
    return;
  }

  const vec3 pos = camera->get_position();
  const vec3 dir = camera->get_forward();

  map.query_visible(pos, dir, HALF_PI, HALF_PI, tree, DEFAULT_RECURSION_DEPTH);
}

//==============================================================================
const MaterialHandle& GraphicsSystem::get_solid_material() const
{
  return m_solid_material;
}

//==============================================================================
const Model& GraphicsSystem::get_cube_model() const
{
  return m_cube_model;
}

//==============================================================================
const DebugCamera* GraphicsSystem::get_camera() const
{
  auto& things = get_engine().get_module<ThingSystem>();
  return things.get_player()->get_camera();
}

//==============================================================================
const mat4 GraphicsSystem::get_default_projection() const
{
  return m_default_projection;
}

//==============================================================================
void GraphicsSystem::update(f32 delta_seconds)
{
  // TODO: only temporary for debug camera
  //m_debug_camera.handle_input(delta_seconds);
  //if (auto* camera = this->get_camera(); camera && CVars::debug_player_raycasts)
  //{
  //  const auto& map = get_engine().get_map();

  //  constexpr f32 RAY_LEN = 10.0f;

  //  const vec3 eye_pos  = camera->get_position();
  //  const vec3 look_dir = camera->get_forward();

  //  const vec3 ray_start = eye_pos;
  //  const vec3 ray_end   = eye_pos + look_dir * RAY_LEN;

  //  // first intersect the map
  //  color4 col = colors::RED;

  //  vec3 out_normal;
  //  f32  out_coeff = FLT_MAX;
  //  if (!map.raycast3d(ray_start, ray_end, out_normal, out_coeff))
  //  {
  //    out_coeff = FLT_MAX;
  //  }

  //  // then intersect enemies
  //  const auto& thing_system = ThingSystem::get();
  //  for (const auto& enemy : thing_system.get_enemies())
  //  {
  //    const f32   width    = enemy.get_width();
  //    const f32   height   = enemy.get_height() * 2.0f;
  //    const vec3  position = enemy.get_position();

  //    const aabb3 bbox = aabb3
  //    {
  //      position - vec3{width, 0.0f,   width},
  //      position + vec3{width, height, width}
  //    };

  //    f32  out;
  //    vec3 normal;
  //    if (intersect::ray_aabb3(ray_start, ray_end, bbox, out, normal) && out < out_coeff)
  //    {
  //      out_coeff  = out;
  //      out_normal = normal;
  //      col        = colors::GREEN;
  //    }

  //    f32  dummy;
  //    vec3 dummy_n;
  //    intersect::ray_aabb3(ray_start, ray_end, bbox, dummy, dummy_n);

  //    if (out_coeff != FLT_MAX)
  //    {
  //      const vec3 hit_pt = eye_pos + look_dir * RAY_LEN * out_coeff;
  //      Gizmo::create_line(0.25f, hit_pt, hit_pt + out_normal, col);
  //    }
  //  }
  //}

  GizmoManager::instance().update_ttls(delta_seconds);
}

//==============================================================================
void GraphicsSystem::render()
{
  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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

  VisibilityTree visible_sectors;
  query_visibility(visible_sectors);

  if (CVars::enable_top_down_debug)
  {
    // Top down rendering for easier debugging
#ifdef NC_DEBUG_DRAW
    render_map_top_down(visible_sectors);
#endif
  }
  else
  {
    const DebugCamera* camera = get_camera();
    if (camera)
    {
      const CameraData camera_data = CameraData
      {
        .position = camera->get_position(),
        .view = camera->get_view(),
        .projection = m_default_projection,
        .vis_tree = visible_sectors,
      };

      GizmoManager::instance().draw_gizmos();
      render_sectors(camera_data);
      render_entities(camera_data);
      render_portals(camera_data);
      render_gun(camera_data);
    }
  }

#ifdef NC_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
#ifdef NC_DEBUG_DRAW
void GraphicsSystem::render_map_top_down(const VisibilityTree& visible_sectors)
{
  glClear(GL_STENCIL_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  static vec2 pointed_position = vec2{ 0.0f };
  static vec2 player_direction = vec2{ 1, 0 };

  static bool show_sector_frustums = true;
  static bool show_visible_sectors = true;
  static bool show_sector_ids = false;
  static bool raycast2d_debug = false;
  static bool inspect_nucledian_portals = false;

  static vec2 raycast_pt1 = vec2{ 0 };
  static f32  raycast_rot = 0.0f;
  static f32  raycast_len = 10.0f;
  static f32  raycast_expand = 0.25f;

  static f32 zoom = 0.04f;

  if (auto* camera = this->get_camera())
  {
    pointed_position = vec2{ camera->get_position().x, camera->get_position().z };
    const auto frwd = vec2{ camera->get_forward().x,  camera->get_forward().z };
    player_direction = is_zero(frwd) ? vec2{ 1, 0 } : normalize(frwd);
  }

  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);
  const f32 aspect = (f32)width / height;

  auto level_space_to_screen_space = [&](vec2 pos) -> vec2
  {
    return vec2{ -1.0f, aspect } *(pos - pointed_position) * zoom;
  };

  auto draw_player = [&](vec2 coords, vec2 dir, vec3 color1, vec3 color2, f32 scale)
  {
    const vec2 front = level_space_to_screen_space(coords + dir * scale);
    const vec2 back = level_space_to_screen_space(coords - dir * 0.5f * scale);
    const vec2 left = level_space_to_screen_space(coords + flipped(dir) * scale * 0.5f - dir * scale);
    const vec2 right = level_space_to_screen_space(coords - flipped(dir) * scale * 0.5f - dir * scale);

    debug_helpers::draw_triangle(front, back, left, color1);
    debug_helpers::draw_triangle(front, back, right, color1);

    debug_helpers::draw_line(front, left, color2);
    debug_helpers::draw_line(left, back, color2);
    debug_helpers::draw_line(back, right, color2);
    debug_helpers::draw_line(right, front, color2);
  };

  if (ImGui::Begin("2D Top Down Debug"))
  {
    ImGui::SliderFloat("Zoom", &zoom, 0.01f, 1.0f);
    ImGui::Separator();
    ImGui::Checkbox("Show visible sectors", &show_visible_sectors);
    ImGui::Checkbox("Show sector frustums", &show_sector_frustums);
    ImGui::Checkbox("Show sector IDs",      &show_sector_ids);
    ImGui::Separator();
    ImGui::Checkbox("Raycast 2D Debug",          &raycast2d_debug);
    ImGui::Checkbox("Inspect nuclidean portals", &inspect_nucledian_portals);

    if (raycast2d_debug)
    {
      ImGui::SliderFloat2("Raycast Point", &raycast_pt1.x,  -40.0f, 40.0f);
      ImGui::SliderFloat("Raycast Dir",    &raycast_rot,    0.0f,   2.0f * PI);
      ImGui::SliderFloat("Raycast Len",    &raycast_len,    0.25f,  30.0f);
      ImGui::SliderFloat("Raycast Expand", &raycast_expand, 0.001f, 3.0f);
    }
  }
  ImGui::End();

  auto& map = get_engine().get_map();

  // Render the floors of the visible_sectors with black or gray if visible_sectors
  for (SectorID i = 0; i < map.sectors.size(); ++i)
  {
    const auto& sector = map.sectors[i];
    const auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    nc_assert(wall_count >= 3);

    const bool is_visible = visible_sectors.is_visible(i);
    const vec3 color = (is_visible && show_visible_sectors) ? vec3{ 0.25f } : vec3{ colors::BLACK };

    const auto& first_wall = map.walls[repr.first_wall];
    vec2 avg_position = first_wall.pos;

    for (WallID index = 1; index < wall_count; ++index)
    {
      WallID next_index = (index + 1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr = repr.first_wall + next_index;
      nc_assert(index_in_arr < map.walls.size());
      nc_assert(next_in_arr < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      avg_position = avg_position + wall1.pos;

      debug_helpers::draw_triangle(
        level_space_to_screen_space(first_wall.pos),
        level_space_to_screen_space(wall1.pos),
        level_space_to_screen_space(wall2.pos),
        color
      );
    }

    avg_position = avg_position / static_cast<f32>(wall_count);
    if (show_sector_ids)
    {
      const auto sector_color = is_visible ? colors::WHITE : colors::PINK;
      const auto sector_str = std::to_string(i);
      debug_helpers::draw_text
      (
        level_space_to_screen_space(avg_position),
        sector_str.c_str(),
        sector_color
      );
    }
  }

  // Render frustums for all visible_sectors visible_sectors
  if (show_sector_frustums)
  {
    glEnable(GL_STENCIL_TEST);

    for (const auto& [sector_id, frustums] : visible_sectors.sectors)
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
        const auto& repr = sector.int_data;
        const s32 wall_count = repr.last_wall - repr.first_wall;
        nc_assert(wall_count >= 3);

        const auto& first_wall = map.walls[repr.first_wall];

        for (WallID index = 1; index < wall_count; ++index)
        {
          WallID next_index = (index + 1) % wall_count;
          WallID index_in_arr = repr.first_wall + index;
          WallID next_in_arr = repr.first_wall + next_index;
          nc_assert(index_in_arr < map.walls.size());
          nc_assert(next_in_arr < map.walls.size());

          const auto& wall1 = map.walls[index_in_arr];
          const auto& wall2 = map.walls[next_in_arr];
          debug_helpers::draw_triangle(
            level_space_to_screen_space(first_wall.pos),
            level_space_to_screen_space(wall1.pos),
            level_space_to_screen_space(wall2.pos),
            vec3{ 1 }
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

          const auto color = vec3{ 0.5f, 0.5f, 0.5f };
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
    constexpr color4 PORTAL_TYPES_COLORS[3] = {colors::WHITE, colors::RED, colors::GREEN};

    auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    nc_assert(wall_count >= 0);

    for (WallRelID index = 0; index < wall_count; ++index)
    {
      WallRelID next_index   = (index + 1) % wall_count;
      WallID    index_in_arr = repr.first_wall + index;
      WallID    next_in_arr  = repr.first_wall + next_index;
      nc_assert(index_in_arr < map.walls.size());
      nc_assert(next_in_arr < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      const auto portal_type = wall1.get_portal_type();
      const auto color = PORTAL_TYPES_COLORS[portal_type];

      debug_helpers::draw_line(
        level_space_to_screen_space(wall1.pos),
        level_space_to_screen_space(wall2.pos),
        color
      );
    }
  }

  // and render the player
  draw_player
  (
    pointed_position,
    player_direction,
    colors::BLACK,
    colors::ORANGE,
    0.5f
  );

  // raycast 2D debug
  if (raycast2d_debug)
  {
    vec2 out_n;
    f32  out_t;

    vec2 start_pt = raycast_pt1;
    vec2 end_pt = raycast_pt1 + vec2{ std::cos(raycast_rot), std::sin(raycast_rot) } *raycast_len;

    debug_helpers::draw_line
    (
      level_space_to_screen_space(start_pt),
      level_space_to_screen_space(end_pt),
      colors::WHITE
    );

    if (map.raycast2d_expanded(start_pt, end_pt, std::max(raycast_expand, 0.0001f), out_n, out_t))
    {
      const vec2 contact_pt = start_pt + (end_pt - raycast_pt1) * out_t;
      end_pt = contact_pt;

      debug_helpers::draw_line
      (
        level_space_to_screen_space(contact_pt + flipped(out_n) * 2.0f),
        level_space_to_screen_space(contact_pt - flipped(out_n) * 2.0f),
        colors::LIME
      );

      debug_helpers::draw_line
      (
        level_space_to_screen_space(contact_pt),
        level_space_to_screen_space(contact_pt + out_n * 1.5f),
        colors::ORANGE
      );
    }

    debug_helpers::draw_line
    (
      level_space_to_screen_space(start_pt),
      level_space_to_screen_space(end_pt),
      colors::GREEN
    );
  }

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
  auto  it = range_list.find(name);

  const auto& range = it != range_list.end() ? it->second : CVars::DEFAULT_RANGE;

  std::visit([&](auto&& t) { draw_cvar_type_and_input(t, range); }, cvar.ptr);
}

//==============================================================================
static void draw_cvar_bar()
{
  auto& cvar_list = CVars::get_cvar_list();

  if (ImGui::BeginTable("Cvar List", 3, ImGuiTableFlags_Borders))
  {
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();

    for (const auto& [name, cvar] : cvar_list)
    {
      ImGui::TableNextRow();
      draw_cvar_row(name, cvar);
    }

    ImGui::EndTable();
  }
}

//==============================================================================
static void draw_keybinds_bar()
{
  constexpr auto KEYBINDS = std::array
  {
    std::pair{"~",   "Show/Hide Debug Window"},
    std::pair{"ESC", "Enable/Disable Cursor And Player Inputs"},
    std::pair{"F1",  "Cycle Between Time Speed 0 and 1"},
    std::pair{"F4",  "Top Down Debug"},
  };

  if (ImGui::BeginTable("Keybindings", 2, ImGuiTableFlags_Borders))
  {
    ImGui::TableSetupColumn("Keybind");
    ImGui::TableSetupColumn("Action");

    ImGui::TableHeadersRow();

    for (const auto& [key, action] : KEYBINDS)
    {
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      ImGui::TextUnformatted(key);

      ImGui::TableNextColumn();
      ImGui::TextUnformatted(action);
    }

    ImGui::EndTable();
  }
}

//==============================================================================
void GraphicsSystem::draw_debug_window()
{
  if (CVars::display_imgui_demo)
  {
    ImGui::ShowDemoWindow(&CVars::display_imgui_demo);
  }

  if (ImGui::Begin("Debug Window", &CVars::display_debug_window))
  {
    if (ImGui::BeginTabBar("Tab Bar"))
    {
      if (ImGui::BeginTabItem("CVars"))
      {
        draw_cvar_bar();
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Keybinds"))
      {
        draw_keybinds_bar();
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}
#endif

//==============================================================================
void GraphicsSystem::render_sectors(const CameraData& camera_data) const
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

  const auto& sectors_to_render = camera_data.vis_tree.sectors;

  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, camera_data.projection);
  m_solid_material.set_uniform(shaders::solid::UNLIT, false);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera_data.view);
  m_solid_material.set_uniform(shaders::solid::VIEW_POSITION, camera_data.position);

  for (const auto& [sector_id, _] : sectors_to_render)
  {
    nc_assert(sector_id < g_sector_meshes.size());
    const MeshHandle& mesh = g_sector_meshes[sector_id];

    glBindVertexArray(mesh.get_vao());

    const color4& color = SECTOR_COLORS[sector_id % SECTOR_COLORS.size()];
    m_solid_material.set_uniform(shaders::solid::COLOR, color);
    m_solid_material.set_uniform(shaders::solid::TRANSFORM, mat4(1.0f));

    glDrawArrays(mesh.get_draw_mode(), 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
}

//==============================================================================
void GraphicsSystem::render_entities(const CameraData& camera_data) const
{
  /*
   * 1. obtain visible_sectors visible_sectors from sector system (TODO)
   * 2. get RenderComponents from entities within visible_sectors visible_sectors (TODO)
   * 3. filer visible_sectors entities (TODO)
   * 4. group by ModelHandle
   * 5. sort groups by: 1. program, 2. texture, 3. VAO (TODO)
   * 5. issue render command for each group (TODO)
   */

  struct ModelGroup
  {
    Model                model;
    std::vector<Entity*> entities; // for now, we keep only enemies here
  };

  std::unordered_map<u64, ModelGroup> model_groups;

  auto& es = ThingSystem::get().get_entities();
  es.for_each<Enemy>([&](Enemy& enemy)
  {
    const Appearance& appearance = enemy.get_appearance();

    // TODO: Use better unique identifier.
    u64 id = (static_cast<u64>(appearance.model.material.m_shader_program) << 32)
      + static_cast<u64>(appearance.model.mesh.get_vao());

    model_groups[id].model = appearance.model;
    model_groups[id].entities.push_back(&enemy);
  });

  // TODO: sort groups by: 1. program, 2. texture, 3. VAO
  for (const auto& [_, group] : model_groups)
  {
    const auto& [model, entities] = group;

    // TODO: switch program & VAO only when necessary
    model.material.use();
    glBindVertexArray(model.mesh.get_vao());

    // TODO: some uniform locations should be shader independent
    model.material.set_uniform(shaders::solid::PROJECTION, camera_data.projection);
    model.material.set_uniform(shaders::solid::UNLIT, false);
    model.material.set_uniform(shaders::solid::VIEW, camera_data.view);
    model.material.set_uniform(shaders::solid::VIEW_POSITION, camera_data.position);

    // TODO: indirect rendering
    for (auto* entity : entities)
    {
      auto* enemy = static_cast<Enemy*>(entity);
      auto        transform  = enemy->calc_transform();
      const auto& appearance = enemy->get_appearance();

      const mat4 transform_matrix = transform.get_matrix() * appearance.transform.get_matrix();

      // TODO: should be set only when these changes and probably not here
      model.material.set_uniform(shaders::solid::COLOR, appearance.color);
      model.material.set_uniform(shaders::solid::TRANSFORM, transform_matrix);

      glDrawArrays(model.mesh.get_draw_mode(), 0, model.mesh.get_vertex_count());
    }
  }

  glBindVertexArray(0);
}

//==============================================================================
void GraphicsSystem::render_portals(const CameraData& camera_data) const
{
  const MapSectors& map = get_engine().get_map();

  glEnable(GL_STENCIL_TEST);

  for (const auto& subtree : camera_data.vis_tree.children)
  {
    const auto portal_id = subtree.portal_wall;
    const auto& wall = map.walls[portal_id];
    nc_assert(wall.get_portal_type() == PortalType::non_euclidean);
    nc_assert(wall.render_data_index != INVALID_PORTAL_RENDER_ID);
    const PortalRenderData& render_data = map.portals_render_data[wall.render_data_index];

    const CameraData new_cam_data
    {
      .position   = camera_data.position,
      .view       = camera_data.view,
      .projection = camera_data.projection,
      .vis_tree   = subtree,
    };

    render_portal(new_cam_data, render_data, 0);
  }
  
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_STENCIL_TEST);
}

//==============================================================================
void GraphicsSystem::render_gun(const CameraData& camera_data) const
{
  const mat4 transform = inverse(camera_data.view) * m_gun_transform.get_matrix();

  m_gun_model.material.use();
  m_gun_model.material.set_uniform(shaders::solid::PROJECTION, camera_data.projection);
  m_gun_model.material.set_uniform(shaders::solid::UNLIT, false);
  m_gun_model.material.set_uniform(shaders::solid::VIEW, camera_data.view);
  m_gun_model.material.set_uniform(shaders::solid::VIEW_POSITION, camera_data.position);
  m_gun_model.material.set_uniform(shaders::solid::COLOR, colors::BROWN);
  m_gun_model.material.set_uniform(shaders::solid::TRANSFORM, transform);

  glClear(GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(m_gun_model.mesh.get_vao());
  glDrawArrays(m_gun_model.mesh.get_draw_mode(), 0, m_gun_model.mesh.get_vertex_count());
  glBindVertexArray(0);
}

//==============================================================================
void GraphicsSystem::render_portal_to_stencil(const CameraData& camera_data, const PortalRenderData& portal, u8 recursion_depth) const
{
  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, camera_data.projection);
  m_solid_material.set_uniform(shaders::solid::UNLIT, true);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera_data.view);
  m_solid_material.set_uniform(shaders::solid::TRANSFORM, portal.transform);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_LEQUAL, recursion_depth, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

  const MeshHandle& quad = MeshManager::instance().get_quad();
  glBindVertexArray(quad.get_vao());
  glDrawArrays(quad.get_draw_mode(), 0, quad.get_vertex_count());
  glBindVertexArray(0);
}

//==============================================================================
void GraphicsSystem::render_portal_to_color(const CameraData& camera_data, const PortalRenderData&, u8 recursion_depth) const
{
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_LEQUAL, recursion_depth + 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  render_sectors(camera_data);
  render_entities(camera_data);
}

//==============================================================================
void GraphicsSystem::render_portal_to_depth(const CameraData& camera_data, const PortalRenderData& portal) const
{
  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, camera_data.projection);
  m_solid_material.set_uniform(shaders::solid::UNLIT, true);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera_data.view);
  m_solid_material.set_uniform(shaders::solid::TRANSFORM, portal.transform);
  m_solid_material.set_uniform(shaders::solid::COLOR, colors::LIME);

  glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthFunc(GL_ALWAYS);

  const MeshHandle& quad = MeshManager::instance().get_quad();
  glBindVertexArray(quad.get_vao());
  glDrawArrays(quad.get_draw_mode(), 0, quad.get_vertex_count());
  glBindVertexArray(0);

  glDepthFunc(GL_LESS);
}

//==============================================================================
void GraphicsSystem::render_portal(const CameraData& camera_data, const PortalRenderData& portal, u8 recursion_depth) const
{
  const MapSectors& map = get_engine().get_map();
  const mat4 virtual_view = camera_data.view * portal.dest_to_src;
  [[maybe_unused]]const vec3 virtual_view_pos = vec3(inverse(virtual_view) * vec4(0.0f, 0.0f, 0.0f, 1.0f));

  const CameraData virtual_camera_data = CameraData
  {
    .position = camera_data.position,
    .view = virtual_view,
    .projection = camera_data.projection,
    .vis_tree = camera_data.vis_tree,
  };
  CameraData clipped_virtual_camera_data = CameraData
  {
    .position = camera_data.position,
    .view = virtual_view,
    .projection = clip_projection(virtual_camera_data, portal),
    .vis_tree = camera_data.vis_tree,
  };

  render_portal_to_stencil(camera_data, portal, recursion_depth);
  render_portal_to_color(clipped_virtual_camera_data, portal, recursion_depth);

  
  for (const auto& subtree : camera_data.vis_tree.children)
  {
    const WallData& wall = map.walls[subtree.portal_wall];
    nc_assert(wall.get_portal_type() == PortalType::non_euclidean);

    const PortalRenderData& render_data = map.portals_render_data[wall.render_data_index];
    const CameraData new_cam_data
    {
      .position   = virtual_camera_data.position,
      .view       = virtual_camera_data.view,
      .projection = virtual_camera_data.projection,
      .vis_tree   = subtree,
    };

    render_portal(new_cam_data, render_data, recursion_depth + 1);
  }
  glStencilFunc(GL_LEQUAL, recursion_depth + 1, 0xFF);

  render_portal_to_depth(camera_data, portal);
}

//==============================================================================
mat4 GraphicsSystem::clip_projection(const CameraData& camera_data, const PortalRenderData& portal) const
{
  // following code was copied from:
  // https://github.com/ThomasRinsma/opengl-game-test/blob/8363bbfcce30acc458b8faacc54c199279092f81/src/sceneobject/portal.cc
  // referenced by following article:
  // https://th0mas.nl/2013/05/19/rendering-recursive-portals-with-opengl/

  const vec3 normal = angleAxis(portal.rotation, VEC3_Y) * -VEC3_Z;
  const vec4 clip_plane = inverse(transpose(camera_data.view)) * vec4(normal, length(portal.position));

  if (clip_plane.w > 0.0f)
    return camera_data.projection;

  const vec4 q = inverse(camera_data.projection) * vec4(sign(clip_plane.x), sign(clip_plane.y), 1.0f, 1.0f);
  const vec4 c = clip_plane * (2.0f / (dot(clip_plane, q)));

  return row(camera_data.projection, 2, c - row(camera_data.projection, 3));
}

//==============================================================================
void GraphicsSystem::build_map_gfx() const
{
  const auto& m_map = get_engine().get_map();
  MeshManager& mesh_manager = MeshManager::instance();

  for (SectorID sid = 0; sid < m_map.sectors.size(); ++sid)
  {
    std::vector<vec3> vertices;
    m_map.sector_to_vertices(sid, vertices);

    const f32* vertex_data = &vertices[0].x;
    const u32  values_cnt = static_cast<u32>(vertices.size() * 3);

    const auto mesh = mesh_manager.create(ResLifetime::Game, vertex_data, values_cnt);
    g_sector_meshes.push_back(mesh);
  }
}

}
