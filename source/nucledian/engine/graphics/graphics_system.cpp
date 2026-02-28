// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/renderer.h>
#include <engine/graphics/camera.h>
#include <engine/graphics/debug/gizmo.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/entities/lights.h>
#include <engine/graphics/entities/sky_box.h>
#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/resources/texture.h>

#include <engine/map/map_system.h>
#include <engine/map/map_dynamics.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/game/game_system.h>
#include <engine/player/level_types.h>
#include <engine/player/player.h>

#include <engine/ui/user_interface_module.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>

#ifdef NC_IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/implot.h>
#endif

#include <profiling.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <string>
#include <utility>
#include <format>
#include <numeric> // std::iota

#ifdef NC_DEBUG_DRAW
#include <chrono>

// Only for "draw_export_menu"
#include <game/item_resources.h>
#include <fstream>

// Debug top-down renderer
#include <engine/graphics/debug/top_down_debug.h>

#endif

namespace nc::debug_helpers
{

//==============================================================================
#ifdef NC_PROFILING
static void display_fps_as_title(SDL_Window* window)
{
  const auto delta_time = get_engine().get_delta_time();
  const auto fps_number = 1.0f / delta_time;
  const auto title_str  = std::format
  (
    "Nucledian Game. FPS: {:.2f}, Dt: {:.2f}ms", fps_number, delta_time * 1000.0f
  );

  SDL_SetWindowTitle(window, title_str.c_str());
}
#endif

//==============================================================================
#ifdef NC_DEBUG_DRAW
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
    default: nc_assert(false, "Unknown severity");             break;
  }

  if (numeric_severity < CVars::opengl_debug_severity)
  {
    return;
  }
    
  nc_log("GL Debug Message: '{0}'", message);
}

#endif

}

namespace nc
{

//==============================================================================
#ifdef NC_IMGUI
static void imgui_start_frame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}
#endif

//==============================================================================
// Default ctor and dtor in .cpp instead of the .h as we would get a compile
// time error because the Renderer is not included in .h (but is in .cpp)
GraphicsSystem::GraphicsSystem()  = default;
GraphicsSystem::~GraphicsSystem() = default;

//==============================================================================
EngineModuleId GraphicsSystem::get_module_id()
{
  return EngineModule::graphics_system;
}

//==============================================================================
GraphicsSystem& GraphicsSystem::get()
{
  return get_engine().get_module<GraphicsSystem>();
}

//==============================================================================
bool GraphicsSystem::init()
{
  NC_TODO("Log out an error when the graphics system initialization fails");
  NC_TODO("Terminate the already set-up SDL stuff on failed initialization.");

  // init SDL
  constexpr auto SDL_INIT_FLAGS = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
  constexpr u32  SDL_WIN_FLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
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
    WINDOW_NAME, 
    WIN_POS, 
    WIN_POS, 
    m_window_width,
    m_window_height,
    SDL_WIN_FLAGS
  );
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

#ifdef NC_DEBUG_DRAW
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(&debug_helpers::gl_debug_message, nullptr);
#endif

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_MULTISAMPLE);

  // disable vsync and unlock fps
  SDL_GL_SetSwapInterval(0);

  glLineWidth(5.0f);

  MeshManager::get().init();
  TextureManager::get().load_directory(ResLifetime::Game, "content/textures");

  // init imgui
#ifdef NC_IMGUI
  ImGui::CreateContext();
  ImPlot::CreateContext();

  ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);

  imgui_start_frame();
#endif

  return true;
}

//==============================================================================
PointLight* light_test;

void GraphicsSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::post_init:
    {
      m_renderer = std::make_unique<Renderer>(m_window_width, m_window_height);

#ifdef NC_DEBUG_DRAW
      m_debug_renderer = std::make_unique<TopDownDebugRenderer>(m_window_width, m_window_height);
#endif
    }
    break;

    case ModuleEventType::game_update:
    {
      this->update(event.update.dt);
    }
    break;

    case ModuleEventType::after_map_rebuild:
    {
      create_sector_meshes();
    }
    break;

    case ModuleEventType::render:
    {
      this->render();
    }
    break;

    case ModuleEventType::terminate:
    {
      this->terminate();
    }
    break;
  }
}

//==============================================================================
void GraphicsSystem::terminate()
{
#ifdef NC_IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
#endif

  MeshManager::get().unload(ResLifetime::Game);

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

  const Camera* camera = Camera::get();
  if (!camera)
    return;

  const vec3 pos = camera->get_position();
  const vec3 dir = camera->get_forward();

  f32 aspect = static_cast<f32>(m_window_width) / m_window_height;

  const f32 horizontal_fov = 2.0f * atan(tan(FOV / 2.0f) * aspect);
  const f32 vertical_fov   = FOV;
  map.query_visible
  (
    pos, dir, horizontal_fov, vertical_fov, tree, DEFAULT_RECURSION_DEPTH
  );
}

//==============================================================================
const MeshHandle& GraphicsSystem::get_and_update_sector_mesh(SectorID sid)
{
  nc_assert(sid < m_sector_meshes.size());

  if (m_dirty_sectors[sid])
  {
    std::vector<f32> vertices;
    GameSystem::get().get_map().sector_to_vertices(sid, vertices);

    MeshManager::get().recreate_sector
    (
      m_sector_meshes[sid], vertices.data(), cast<u32>(vertices.size())
    );

    m_dirty_sectors[sid] = false;
  }

  return m_sector_meshes[sid];
}

//==============================================================================
void GraphicsSystem::mark_sector_dirty(SectorID sid)
{
  nc_assert(sid < m_dirty_sectors.size());
  m_dirty_sectors[sid] = true;
}

//==============================================================================
const ShaderProgramHandle& GraphicsSystem::get_solid_material() const
{
  return m_renderer->get_solid_material();
}

//==============================================================================
void GraphicsSystem::update([[maybe_unused]]f32 delta_seconds)
{
#ifdef NC_DEBUG_DRAW
  GizmoManager::get().update_ttls(delta_seconds);
#endif
}

//==============================================================================
static void grab_render_gun_props(RenderGunProperties& props)
{
  GameSystem& game = GameSystem::get();
  const Player* player = game.get_player();

  if (player)
  {
    player->get_gun_props(props);
  }
  else
  {
    props.sprite = "";
    props.sway   = VEC2_ZERO;
  }
}

//==============================================================================
void GraphicsSystem::render()
{
  NC_SCOPE_PROFILER(Render)

  int width = 0, height = 0;
  SDL_GetWindowSize(m_window, &width, &height);

  nc_assert(width >= 0 && height >= 0, "WTF?");

  u32 u_width  = static_cast<u32>(width);
  u32 u_height = static_cast<u32>(height);

  if (u_width != m_window_width || u_height != m_window_height)
  {
    m_window_width  = u_width;
    m_window_height = u_height;
    m_renderer->on_window_resized(m_window_width, m_window_height);
#ifdef NC_DEBUG_DRAW
    m_debug_renderer->on_window_resized(m_window_width, m_window_height);
#endif
  }

#ifdef NC_DEBUG_DRAW
  // Note that this call can take up to 200 microseconds
  debug_helpers::display_fps_as_title(m_window);
#endif

  glViewport(0, 0, width, height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

#ifdef NC_DEBUG_DRAW
  if (CVars::display_debug_window)
  {
    draw_debug_window();
  }

  handle_light_debug();
  handle_sector_height_debug();
#endif

  VisibilityTree visible_sectors;
  query_visibility(visible_sectors);

  RenderGunProperties gun_props;
  grab_render_gun_props(gun_props);

#ifdef NC_DEBUG_DRAW
  if (CVars::enable_top_down_debug)
  {
    // Top down rendering for easier debugging
    m_debug_renderer->render(visible_sectors);
  }
  else
#endif
  {
    m_renderer->render(visible_sectors, gun_props);

    get_engine().get_module<UserInterfaceSystem>().draw_hud();
  }

#ifdef NC_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  imgui_start_frame(); // start a new frame just after rendering the old one
#endif

#ifdef NC_DEBUG_DRAW
  m_debug_renderer->post_render();
#endif

  SDL_GL_SwapWindow(m_window);
}

//==============================================================================
#ifdef NC_DEBUG_DRAW
static void draw_cvar_type_and_input(f32* flt, const CVarRange& range)
{
  ImGui::TableNextColumn();
  ImGui::Text("f32");

  ImGui::TableNextColumn();
  ImGui::PushID(flt);
  ImGui::SliderFloat("", flt, range.min, range.max);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(s32* num, const CVarRange& rn)
{
  ImGui::TableNextColumn();
  ImGui::Text("s32");

  ImGui::TableNextColumn();
  ImGui::PushID(num);
  ImGui::SliderInt("", num, static_cast<s32>(rn.min), static_cast<s32>(rn.max));
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(std::string* str, const CVarRange&)
{
  ImGui::TableNextColumn();
  ImGui::Text("string");

  ImGui::TableNextColumn();
  ImGui::PushID(str);
  ImGui::InputText("", str);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_type_and_input(bool* bl, const CVarRange&)
{
  ImGui::TableNextColumn();
  ImGui::Text("bool");

  ImGui::TableNextColumn();
  ImGui::PushID(bl);
  ImGui::Checkbox("", bl);
  ImGui::PopID();
}

//==============================================================================
static void draw_cvar_row(const std::string& name, const CVar& cvar)
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

  // First build up the categories
  using CVarPair    = std::pair<std::string, CVar>;
  using CategoryMap = std::map<std::string, std::vector<CVarPair>>;

  CategoryMap cvar_categories;
  for (const auto& [name, cvar] : cvar_list)
  {
    // find the first "." in the name and decide it's category based on that
    if (u64 idx = name.find('.'); idx != std::string::npos && idx >= 1)
    {
      std::string category_name = std::string{name.begin(), name.begin() + idx - 1};
      std::string stripped_name = std::string{name.begin() + idx + 1, name.end()};
      cvar_categories[category_name].push_back({stripped_name, cvar});
    }
    else
    {
      // "default" category for cvars without one
      cvar_categories["default"].push_back({name, cvar});
    }
  }

  // And now render all the categories as items in the list..
  // Only one of them will actually render all of the cvars inside
  if (ImGui::BeginTabBar("CVar Types"))
  {
    for (const auto& [category, list] : cvar_categories)
    {
      if (ImGui::BeginTabItem(category.c_str()))
      {
        if (ImGui::BeginTable("Cvar List", 3, ImGuiTableFlags_Borders))
        {
          ImGui::TableSetupColumn("Name");
          ImGui::TableSetupColumn("Type");
          ImGui::TableSetupColumn("Value");
          ImGui::TableHeadersRow();

          for (const auto& [name, cvar] : list)
          {
            ImGui::TableNextRow();
            draw_cvar_row(name, cvar);
          }

          ImGui::EndTable();
        }

        ImGui::EndTabItem();
      }
    }

    ImGui::EndTabBar();
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
static void draw_level_selection()
{
  auto& game = GameSystem::get();

  const auto  curr_lvl = game.get_level_name();

  bool already_selected = false;

  ImGui::Text("Current Level: %s", curr_lvl.to_cstring().data());
  ImGui::Separator();

  if (ImGui::Button("Next Level"))
  {
    game.request_play_level(Levels::LEVEL_2);
    already_selected = true;
  }

  ImGui::Separator();

  for(const auto &id : LevelsDB)
  {
    if (ImGui::Button(id.to_cstring().data()) && !already_selected)
    {
      game.request_play_level(id);
      already_selected = true;
    }
  }
}

//==============================================================================
static void draw_saves_menu()
{
  auto& game  = GameSystem::get();
  auto& saves = game.get_save_game_db();

  if (ImGui::Button("Save Current Game"))
  {
    auto save = game.save_game();
    saves.push_back(GameSystem::SaveDbEntry
    {
      .data  = save,
      .dirty = true,
    });
  }

  ImGui::Separator();

  int to_load = -1;
  int i = 0;
  for (const auto&[save, dirty] : saves)
  {
    namespace ch = std::chrono;

    ImGui::Text("Save IDX[%d]", i);
    ImGui::Text("Save ID [%d]", static_cast<int>(save.id));
    ImGui::Text("Dirty: %s", dirty ? "T" : "F");
    //ImGui::Text("Level: %s", save.last_level.to_cstring().data());

#ifdef NC_MSVC // localtime_s is platform specific
    ch::year_month_day date{ch::floor<ch::days>(save.time)};

    // We use time_t because std::chrono::hh_mm_ss reports this stupid error
    // "N4950 [time.hms.overview]/2 mandates Duration to be a specialization of chrono::duration."
    std::tm     local_tm;
    std::time_t now_c = SaveGameData::Clock::to_time_t(save.time);

    localtime_s(&local_tm, &now_c);

    ImGui::Text
    (
      "Date: %u.%u.%d %d:%d",
      (unsigned)date.day(),
      (unsigned)date.month(),
      (int)     date.year(),
      (int)     local_tm.tm_hour,
      (int)     local_tm.tm_min
    );
#endif

    ImGui::PushID(i);
    const bool should_load = ImGui::Button("Load");
    ImGui::PopID();

    if (should_load)
    {
      to_load = i;
    }

    ImGui::Separator();
    ++i;
  }

  if (to_load >= 0)
  {
    game.load_game(saves[to_load].data);
  }
}

//==============================================================================
static void export_pickups(cstr file_path)
{
  // open the file
  std::ofstream output(file_path);
  if (!output.is_open())
  {
    nc_warn("Could not export pickups to file \"{}\"", file_path);
    return;
  }

  for (u64 i = 0; i < PickupTypes::count; ++i)
  {
    output << PICKUP_NAMES[i] << '\n';
  }

  output.close();
}

//==============================================================================
static void draw_export_menu()
{
  if (ImGui::Button("Export Pickups"))
  {
    export_pickups("nc_pickups_export.txt");
  }
}

//==============================================================================
#ifdef NC_PROFILING
enum class MainPlotType
{
  delta_time = 0,
  num_calls,
};

//==============================================================================
template<MainPlotType TYPE>
static void draw_main_plot()
{
  constexpr ImPlotFlags PLOT_FLAGS
    = ImPlotFlags_NoMouseText
    | ImPlotFlags_NoBoxSelect
    | ImPlotFlags_NoInputs;

  constexpr ImPlotAxisFlags FLAGS_X
    = ImPlotAxisFlags_NoTickLabels
    | ImPlotAxisFlags_NoTickMarks
    | ImPlotAxisFlags_NoGridLines
    | ImPlotAxisFlags_AutoFit;

  constexpr ImPlotAxisFlags FLAGS_Y
    = ImPlotAxisFlags_AutoFit;

  static std::array<f32, Profiler::MAX_SAMPLES> XS_F32 = []()
  {
    std::array<f32, Profiler::MAX_SAMPLES> data;
    std::iota(data.begin(), data.end(), 0.0f);
    return data;
  }();

  static std::array<u32, Profiler::MAX_SAMPLES> XS_U32 = []()
  {
    std::array<u32, Profiler::MAX_SAMPLES> data;
    std::iota(data.begin(), data.end(), 0_u32);
    return data;
  }();

  constexpr cstr NAMES[2] = {"Delta Time Plot", "Number Of Calls Plot"};

  const auto& samples = Profiler::get().get_profiling_data_for_all_scopes();

  if (ImPlot::BeginPlot(NAMES[cast<int>(TYPE)], ImVec2(-1,300), PLOT_FLAGS))
  {
    ImPlot::SetupAxes(nullptr, nullptr, FLAGS_X, FLAGS_Y);

    // Makes the shaded part of the plot transparent
    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);

    // Then render them
    for (const auto&[name, data] : samples)
    {
      // Shaded part
      if constexpr (TYPE == MainPlotType::delta_time)
      {
        ImPlot::PlotShaded
        (
          name.c_str(), XS_F32.data(), data.delta_time.data(),
          cast<int>(XS_F32.size()), -INFINITY, 0
        );
      }
      else
      {
        ImPlot::PlotShaded
        (
          name.c_str(), XS_U32.data(), data.num_calls.data(),
          cast<int>(XS_U32.size()), -INFINITY, 0
        );
      }
    }

    // Plot the current position in the frame
    u64 idx = get_engine().get_frame_idx() & (Profiler::MAX_SAMPLES - 1);

    if constexpr (TYPE == MainPlotType::delta_time)
    {
      f32 ver_line_x = cast<f32>(idx);
      ImPlot::PlotInfLines("##InfLine", &ver_line_x, 1);
    }
    else
    {
      ImPlot::PlotInfLines("##InfLine", &idx, 1);
    }

    ImPlot::EndPlot();
  }
}

//==============================================================================
void draw_profiling()
{
  NC_SCOPE_PROFILER(DrawProfiler)

  static bool draw_delta_time = false;
  static bool draw_num_calls  = false;
  
  ImGui::Checkbox("Plot Delta Time", &draw_delta_time);
  ImGui::SameLine();
  ImGui::Checkbox("Plot Num Calls", &draw_num_calls);
  ImGui::Separator();

  if (draw_delta_time)
  {
    draw_main_plot<MainPlotType::delta_time>();
  }

  if (draw_num_calls)
  {
    draw_main_plot<MainPlotType::num_calls>();
  }
}
#endif

//==============================================================================
void GraphicsSystem::handle_light_debug()
{
  if (CVars::light_debug)
  {
    if (ImGui::Begin("Light Debug", &CVars::light_debug))
    {
      vec3 pos = light_test->get_position();
      if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
      {
        light_test->set_position(pos);
      }

      bool changed = false;

      changed |= ImGui::ColorEdit3("Color",    &light_test->color.x);
      changed |= ImGui::DragFloat("Intensity", &light_test->intensity, 0.001f, 0.0f, 10.0f);
      changed |= ImGui::DragFloat("Radius",    &light_test->radius, 0.001f, 0.0f, 32.0f);
      changed |= ImGui::DragFloat("Falloff",   &light_test->falloff, 0.001f, 0.01f, 10.0f);

      if (changed)
      {
        light_test->refresh_entity_radius();
      }

      ImGui::Separator();
      Player* p = GameSystem::get().get_player();
      if (p)
      {
        f32 dist = length(p->get_position() - light_test->get_position());
        ImGui::Text("Distance to player is: %.2f", dist);
      }
      ImGui::Text("Light radius is: %.2f", light_test->get_radius());
    }

    ImGui::End();
  }
}

//==============================================================================
void GraphicsSystem::handle_sector_height_debug()
{
  if (CVars::sector_height_debug)
  {
    MapSectors& map = const_cast<MapSectors&>(GameSystem::get().get_map());

    if (ImGui::Begin("Runtime map changes debug"))
    {
      static int modify = -1;
      ImGui::DragInt("Sector to modify", &modify, 1.0f, -1, cast<int>(map.sectors.size()) - 1);

      if (modify != -1)
      {
        ImGui::Text("Selected sector %d", modify);
        SectorData& sd = map.sectors[modify];

        bool changed = false;

        changed |= ImGui::DragFloat("Floor height", &sd.floor_height, 0.01f, 0.0f, 100.0f);
        changed |= ImGui::DragFloat("Ceil height",  &sd.ceil_height,  0.01f, 0.0f, 100.0f);
        sd.ceil_height = max(sd.floor_height + 0.1f, sd.ceil_height);

        if (changed)
        {
          SectorID sid = cast<SectorID>(modify);
          GraphicsSystem::get().mark_sector_dirty(sid);

          map.for_each_portal_of_sector(sid, [&](WallID wall)
          {
            SectorID neighbor = map.walls[wall].portal_sector_id;
            nc_assert(map.is_valid_sector_id(neighbor));
            GraphicsSystem::get().mark_sector_dirty(neighbor);
          });
        }
      }
    }

    ImGui::End();

    MapDynamics& dynamics = GameSystem::get().get_map_dynamics();
    std::vector<u16> activator_values(dynamics.activators.size());
    dynamics.evaluate_activators(activator_values);

    for (u64 idx = 0; idx < activator_values.size(); ++idx)
    {
      int value  = cast<int>(activator_values[idx]);
      int thresh = cast<int>(dynamics.activators[idx].threshold);
      int scount = cast<int>(dynamics.activators[idx].affected_sectors.size());

      ImGui::PushID(cast<int>(idx));
      ImGui::Text("Value/Threshold/SectorCNT: %d/%d/%d", value, thresh, scount);
      ImGui::PopID();
    }
  }
}

//==============================================================================
void GraphicsSystem::draw_debug_window()
{
  if (CVars::display_imgui_demo)
  {
    ImGui::ShowDemoWindow(&CVars::display_imgui_demo);
    ImPlot::ShowDemoWindow(&CVars::display_imgui_demo);
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

      if (ImGui::BeginTabItem("Levels"))
      {
        draw_level_selection();
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Saves"))
      {
        draw_saves_menu();
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Export"))
      {
        draw_export_menu();
        ImGui::EndTabItem();
      }

#ifdef NC_PROFILING
      if (ImGui::BeginTabItem("Profiling"))
      {
        draw_profiling();
        ImGui::EndTabItem();
      }
#endif

      ImGui::EndTabBar();
    }
  }
  ImGui::End();
}
#endif

//==============================================================================
void GraphicsSystem::create_sector_meshes()
{
  const MapSectors& map          = GameSystem::get().get_map();
  MeshManager&      mesh_manager = MeshManager::get();
  std::vector<f32>  vertices;

  m_sector_meshes.clear();
  m_dirty_sectors.clear();
  m_sector_meshes.reserve(map.sectors.size());
  m_dirty_sectors.resize(map.sectors.size(), false);

  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
  {
    vertices.clear();
    map.sector_to_vertices(sector_id, vertices);

    const auto mesh = mesh_manager.create_sector
    (
      ResLifetime::Level,
      vertices.data(),
      static_cast<u32>(vertices.size())
    );
    m_sector_meshes.push_back(mesh);
  }
}

//==============================================================================
vec2 GraphicsSystem::get_window_size()
{
  return vec2(m_window_width, m_window_height);
}

//==============================================================================
SDL_Window* GraphicsSystem::get_window()
{
  return m_window;
}

}
