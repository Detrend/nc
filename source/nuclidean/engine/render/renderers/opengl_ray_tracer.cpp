// Project Nuclidean Source File
#include <engine/render/renderers/opengl_ray_tracer.h>

#include <common.h>
#include <config.h>
#include <logging.h>
#include <math/matrix.h>

#include <engine/game/game_system.h>
#include <engine/graphics/camera.h>
#include <engine/map/map_system.h>
#include <engine/render/render_system.h>
#include <engine/render/sdl_utils.h>
#include <engine/render/opengl/utils.h>
#include <engine/render/texture_atlas.h>
#include <engine/render/shaders/tracing.shared.h>

#include <shaders/tracing.h>

#include <glad/glad.h>

#if NC_IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#endif

#include <SDL_video.h>

#include <cmath>
#include <cstddef>
#include <iterator>
#include <vector>

namespace nc::render::renderers
{

//==============================================================================
template<typename T>
static void upload_ssbo(GLuint buffer, const std::vector<T>& data, GLenum usage)
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, cast<GLsizeiptr>(data.size() * sizeof(T)), data.data(), usage);
}

//==============================================================================
static Surface make_surface(const SurfaceData& surface, bool triggered)
{
  const TextureID texture_id = triggered ? surface.texture_id_triggered : surface.texture_id_default;
  return Surface
  {
    .offset     = surface.offset,
    .texture_id = surface.should_show ? cast<u32>(texture_id) : INVALID,
  };
}

//==============================================================================
static WallSegment make_wall_segment(const WallSegmentData& segment, bool triggered)
{
  return WallSegment
  {
    .surface    = make_surface(segment.surface, triggered),
    .end_height = segment.end_height,
  };
}

//==============================================================================
// Whether the portal opening covers the whole wall at the default heights.
static bool is_portal_fully_open(const MapSectors& map, const SectorData& sector, const WallData& wall)
{
  const SectorData& neighbor = map.sectors[wall.portal_sector_id];
  const f32 floor_y = sector.state_floors[0];
  const f32 ceil_y = sector.state_ceils[0];

  // neighbor heights as seen from the sector, non-euclidean portals line the floors up
  f32 neighbor_floor_y = neighbor.state_floors[0];
  f32 neighbor_ceil_y = neighbor.state_ceils[0];
  if (wall.get_portal_type() == PortalType::non_euclidean)
  {
    neighbor_floor_y = floor_y;
    neighbor_ceil_y = floor_y + neighbor.state_ceils[0] - neighbor.state_floors[0];
  }

  const bool overlaps = neighbor_floor_y < ceil_y && neighbor_ceil_y > floor_y;
  return overlaps && neighbor_floor_y <= floor_y && neighbor_ceil_y >= ceil_y;
}

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
  glGenBuffers(1, &m_context_buffer);
  glGenBuffers(1, &m_walls_buffer);
  glGenBuffers(1, &m_sectors_buffer);
  glGenBuffers(1, &m_portals_buffer);

#if NC_DEBUG_DRAW
  glGenQueries(cast<GLsizei>(std::size(m_time_queries)), m_time_queries);
#endif

  const TextureAtlas& atlas = render_system.get_game_atlas();
  const GLsizei atlas_width = cast<GLsizei>(atlas.get_size().x);
  const GLsizei atlas_height = cast<GLsizei>(atlas.get_size().y);
  glGenTextures(1, &m_atlas_texture);
  glBindTexture(GL_TEXTURE_2D, m_atlas_texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, atlas_width, atlas_height);
  glTexSubImage2D
  (
    GL_TEXTURE_2D, 0, 0, 0, atlas_width, atlas_height, GL_RGBA, GL_UNSIGNED_BYTE, atlas.get_texels().data()
  );

  glGenBuffers(1, &m_textures_buffer);
  glGenBuffers(1, &m_wall_segments_buffer);
  upload_ssbo(m_textures_buffer, atlas.get_textures(), GL_STATIC_DRAW);

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
void OpenGLRayTracer::on_map_rebuilt()
{
  const MapSectors& map = GameSystem::get().get_map();

  std::vector<Sector> sectors;
  std::vector<Wall> walls(map.walls.size());
  std::vector<Portal> portals;

  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
  {
    const SectorData& sector = map.sectors[sector_id];
    sectors.push_back(Sector
    {
      .walls_offset    = sector.first_wall,
      .walls_count     = cast<u32>(sector.last_wall - sector.first_wall),
      .floor_y         = map.sectors_dynamic[sector_id].floor_height,
      .ceil_y          = map.sectors_dynamic[sector_id].ceil_height,
      .default_floor_y = sector.state_floors[0],
      .default_ceil_y  = sector.state_ceils[0],
      .floor           = make_surface(sector.floor_surface, false),
      .ceiling         = make_surface(sector.ceil_surface, false),
    });

    f32 cumulative_length = 0.0f;
    for (WallID wall_id = sector.first_wall; wall_id < sector.last_wall; ++wall_id)
    {
      const WallData& wall = map.walls[wall_id];
      walls[wall_id] = Wall
      {
        .position          = wall.pos,
        .portal_id         = wall.is_portal() ? cast<u32>(portals.size()) : INVALID,
        .cumulative_length = cumulative_length,
        .segments_offset   = wall.first_segment,
        .segments_count    = wall.segment_count,
      };

      // levels align their textures to the legacy renderer, which skipped the length of fully open portals
      if (!wall.is_portal() || !is_portal_fully_open(map, sector, wall))
        cumulative_length += distance(wall.pos, map.walls[map_helpers::next_wall(map, sector_id, wall_id)].pos);

      if (!wall.is_portal())
        continue;

      // Classic portals keep the ray as is, non-euclidean ones move it to the destination portal.
      mat4 transform = mat4(1.0f);
      if (wall.render_data_index != INVALID_PORTAL_RENDER_ID)
        transform = inverse(map.portals_render_data[wall.render_data_index].dest_to_src);

      portals.push_back(Portal
      {
        .translation = vec3(transform[3]),
        .rotation    = std::atan2(transform[2][0], transform[0][0]),
        .destination_id = wall.portal_sector_id,
      });
    }
  }

  std::vector<WallSegment> wall_segments;
  wall_segments.reserve(map.wall_segments.size());
  for (SegmentID segment_id = 0; segment_id < map.wall_segments.size(); ++segment_id)
  {
    const bool triggered = map.wall_segments_dynamic[segment_id].triggered;
    wall_segments.push_back(make_wall_segment(map.wall_segments[segment_id], triggered));
  }

  upload_ssbo(m_sectors_buffer, sectors, GL_DYNAMIC_DRAW);
  upload_ssbo(m_walls_buffer, walls, GL_STATIC_DRAW);
  upload_ssbo(m_portals_buffer, portals, GL_STATIC_DRAW);
  upload_ssbo(m_wall_segments_buffer, wall_segments, GL_STATIC_DRAW);
}

//==============================================================================
void OpenGLRayTracer::on_sector_changed(SectorID sector_id)
{
  const SectorDynData& sector = GameSystem::get().get_map().sectors_dynamic[sector_id];
  const vec2 heights(sector.floor_height, sector.ceil_height);

  // only the heights change, written at once as they are next to each other
  static_assert(offsetof(Sector, ceil_y) == offsetof(Sector, floor_y) + sizeof(f32));
  const GLintptr offset = cast<GLintptr>(sector_id * sizeof(Sector) + offsetof(Sector, floor_y));
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sectors_buffer);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(heights), &heights);
}

//==============================================================================
void OpenGLRayTracer::on_segment_changed(SegmentID segment_id)
{
  const MapSectors& map = GameSystem::get().get_map();
  const bool triggered = map.wall_segments_dynamic[segment_id].triggered;
  const WallSegment segment = make_wall_segment(map.wall_segments[segment_id], triggered);

  const GLintptr offset = cast<GLintptr>(segment_id * sizeof(WallSegment));
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_wall_segments_buffer);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(segment), &segment);
}

//==============================================================================
void OpenGLRayTracer::render()
{
  const MapSectors& map = GameSystem::get().get_map();
  const Camera* const camera = Camera::get();

  const vec3 camera_position = camera->get_position();
  const SectorID camera_sector_id = map.get_sector_from_point(vec2(camera_position.x, camera_position.z));

  const Context context
  {
    .camera_position  = camera_position,
    .camera_sector_id = camera_sector_id == INVALID_SECTOR_ID ? INVALID : camera_sector_id,
    .camera_direction = camera->get_forward(),
  };

#if NC_DEBUG_DRAW
  // the query was issued a few frames ago, so its result is ready and reading it does not wait for the GPU
  const GLuint time_query = m_time_queries[m_frame_count % std::size(m_time_queries)];
  if (m_frame_count >= std::size(m_time_queries))
  {
    GLuint64 time = 0;
    glGetQueryObjectui64v(time_query, GL_QUERY_RESULT, &time);
    m_gpu_time = cast<f32>(time) / 1'000'000.0f;
  }
  ++m_frame_count;

  glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cast<GLuint>(TracingBuffer::walls), m_walls_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cast<GLuint>(TracingBuffer::sectors), m_sectors_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cast<GLuint>(TracingBuffer::portals), m_portals_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cast<GLuint>(TracingBuffer::wall_segments), m_wall_segments_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cast<GLuint>(TracingBuffer::textures), m_textures_buffer);

  glBindBufferBase(GL_UNIFORM_BUFFER, cast<GLuint>(TracingUniform::context), m_context_buffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(context), &context, GL_DYNAMIC_DRAW);

  glActiveTexture(GL_TEXTURE0 + cast<GLuint>(TracingTexture::atlas));
  glBindTexture(GL_TEXTURE_2D, m_atlas_texture);

  glBindImageTexture(cast<GLuint>(TracingImage::out_color), m_output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  const uvec2 groups = (m_output_resolution + TRACING_GROUP_SIZE - 1u) / TRACING_GROUP_SIZE;
  m_tracing_shader.dispatch(groups.x, groups.y, 1, true);

  const GLint width = cast<GLint>(m_output_resolution.x);
  const GLint height = cast<GLint>(m_output_resolution.y);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_output_framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if NC_DEBUG_DRAW
  glEndQuery(GL_TIME_ELAPSED);
#endif

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
