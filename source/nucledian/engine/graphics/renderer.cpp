// Project Nucledian Source File
#include <engine/graphics/renderer.h>

#include <common.h>
#include <logging.h>
#include <cvars.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/graphics/camera.h>
#include <engine/graphics/debug/gizmo.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/entities/lights.h>
#include <engine/graphics/entities/sky_box.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/core/engine.h>
#include <engine/map/map_system.h>
#include <engine/game/game_system.h>
#include <engine/game/game_helpers.h>
#include <engine/appearance.h>

#include <imgui/imgui.h>

#include <array>
#include <filesystem>
#include <unordered_set>
#include <set>

namespace nc
{

// Returns the <root>/source/nucledian/ prefix used to resolve relative shader paths.
static const std::string& renderer_shader_root()
{
  static const std::string root = []()
  {
    // renderer.cpp lives at <root>/source/nucledian/engine/graphics/renderer.cpp
    // Go up 3 directory components (strip filename + engine/graphics) to reach <root>/source/nucledian/
    std::string path = __FILE__;
    std::replace(path.begin(), path.end(), '\\', '/');
    for (int i = 0; i < 3; ++i)
    {
      const auto pos = path.rfind('/');
      if (pos != std::string::npos)
        path.resize(pos);
    }
    return path + "/";
  }();
  return root;
}

// Returns the newest last_write_time across all source files of an entry.
static std::filesystem::file_time_type newest_write_time(const std::vector<std::string>& file_paths)
{
  auto newest = std::filesystem::file_time_type::min();
  for (const auto& rel : file_paths)
  {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(renderer_shader_root() + rel, ec);
    if (!ec && t > newest)
      newest = t;
  }
  return newest;
}

//==============================================================================
void Renderer::register_shader(ShaderProgramHandle& handle,
                               std::initializer_list<const char*> paths)
{
  ShaderEntry entry;
  entry.handle = &handle;
  for (const char* p : paths)
    entry.file_paths.emplace_back(p);

  // Seed the write time so the first hot-reload check does not false-trigger.
  entry.last_write_time = newest_write_time(entry.file_paths);
  m_shader_entries.push_back(std::move(entry));
}

//==============================================================================
void Renderer::check_shader_hot_reload() const
{
  for (auto& entry : m_shader_entries)
  {
    const auto newest = newest_write_time(entry.file_paths);
    if (newest <= entry.last_write_time)
      continue;

    if (entry.handle->try_reload(entry.file_paths))
    {
      entry.last_write_time = newest;
      nc_log("Shader hot-reloaded: {}", entry.file_paths[0]);
    }
    else
    {
      nc_crit("Shader hot-reload failed: {}", entry.file_paths[0]);
    }
  }
}

//==============================================================================
Renderer::Renderer(u32 win_w, u32 win_h)
: m_solid_material(ShaderProgramHandle::from_files(shaders::solid::VERTEX_FILE, shaders::solid::FRAGMENT_FILE))
, m_billboard_material(ShaderProgramHandle::from_files(shaders::billboard::VERTEX_FILE, shaders::billboard::FRAGMENT_FILE))
, m_gun_material(ShaderProgramHandle::from_files(shaders::gun::VERTEX_FILE, shaders::gun::FRAGMENT_FILE))
, m_light_material(ShaderProgramHandle::from_files(shaders::light::VERTEX_FILE, shaders::light::FRAGMENT_FILE))
, m_sector_material(ShaderProgramHandle::from_files(shaders::sector::VERTEX_FILE, shaders::sector::FRAGMENT_FILE))
, m_light_culling_shader(ShaderProgramHandle::from_file(shaders::light_culling::COMPUTE_FILE))
, m_sky_box_material(ShaderProgramHandle::from_files(shaders::sky_box::VERTEX_FILE, shaders::sky_box::FRAGMENT_FILE))
, m_pixel_light_shader(ShaderProgramHandle::from_files(shaders::pixel_light::VERTEX_FILE, shaders::pixel_light::FRAGMENT_FILE))
, m_window_size(win_w, win_h)
{
  this->create_g_buffers(win_w, win_h);
  this->recompute_projection(win_w, win_h, GraphicsSystem::FOV);

  const vec2 game_atlas_size = TextureManager::get().get_atlas(ResLifetime::Game).get_size();
  const vec2 level_atlas_size = TextureManager::get().get_atlas(ResLifetime::Level).get_size();

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::GAME_ATLAS_SIZE, game_atlas_size);
  m_sector_material.set_uniform(shaders::sector::LEVEL_ATLAS_SIZE, level_atlas_size);

  // setup texture data ssbo
  // TODO: this should be done every time new texture atlas is created
  {
    const std::vector<TextureHandle> textures = TextureManager::get().get_textures();
    m_textures_ssbo = SSBOBuffer<TextureGPU>(textures.size());
    for (auto&& texture : textures)
    {
      m_textures_ssbo.push_back(texture.get_gpu_data());
    }
    m_textures_ssbo.update_gpu_data(true);
  }

  // setup light culling ssbos
  {
    const size_t num_tiles_x = (win_w + LIGHT_CULLING_TILE_SIZE_X - 1) / LIGHT_CULLING_TILE_SIZE_X;
    const size_t num_tiles_y = (win_h + LIGHT_CULLING_TILE_SIZE_Y - 1) / LIGHT_CULLING_TILE_SIZE_Y;
    const size_t num_tiles = num_tiles_x * num_tiles_y;

    m_light_index_ssbo = SSBOBuffer<u32>(MAX_LIGHTS_PER_TILE * num_tiles);
    m_light_tiles_ssbo = SSBOBuffer<u32>(num_tiles * 2);
    m_light_counter_ssbo = SSBOBuffer<u32>(1);
  }

  // Register all shader programs for hot-reload monitoring.
  register_shader(m_solid_material,        {shaders::solid::VERTEX_FILE,        shaders::solid::FRAGMENT_FILE});
  register_shader(m_billboard_material,    {shaders::billboard::VERTEX_FILE,    shaders::billboard::FRAGMENT_FILE});
  register_shader(m_gun_material,          {shaders::gun::VERTEX_FILE,          shaders::gun::FRAGMENT_FILE});
  register_shader(m_light_material,        {shaders::light::VERTEX_FILE,        shaders::light::FRAGMENT_FILE});
  register_shader(m_sector_material,       {shaders::sector::VERTEX_FILE,       shaders::sector::FRAGMENT_FILE});
  register_shader(m_light_culling_shader,  {shaders::light_culling::COMPUTE_FILE});
  register_shader(m_sky_box_material,      {shaders::sky_box::VERTEX_FILE,      shaders::sky_box::FRAGMENT_FILE});
  register_shader(m_pixel_light_shader,    {shaders::pixel_light::VERTEX_FILE,  shaders::pixel_light::FRAGMENT_FILE});
}

//==============================================================================
void Renderer::on_window_resized(u32 w, u32 h)
{
  m_window_size = ivec2{w, h};

  // resize g buffers
  this->destroy_g_buffers();
  this->create_g_buffers(w, h);
  this->recompute_projection(w, h, GraphicsSystem::FOV);

  // resize ssbo buffers
  {
    const size_t num_tiles_x = (w + LIGHT_CULLING_TILE_SIZE_X - 1) / LIGHT_CULLING_TILE_SIZE_X;
    const size_t num_tiles_y = (h + LIGHT_CULLING_TILE_SIZE_Y - 1) / LIGHT_CULLING_TILE_SIZE_Y;
    const size_t num_tiles = num_tiles_x * num_tiles_y;

    m_light_index_ssbo.resize(MAX_LIGHTS_PER_TILE * num_tiles);
    m_light_tiles_ssbo.resize(num_tiles * 2);
  }
}

//==============================================================================
void Renderer::render
(
  const VisibilityTree& visibility_tree,
  const RenderGunProperties& gun_data
)
const
{
  check_shader_hot_reload();

  const Camera* camera = Camera::get();
  if (!camera)
    return;

  const CameraData camera_data = CameraData
  {
    .position = camera->get_position(),
    .view = camera->get_view(),
    .vis_tree = visibility_tree,
    .portal_dest_to_src = mat4(1.0f),
  };

  float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  glClearTexImage(
    GraphicsSystem::get().megatex_handle,        // texture id
    0,              // mip level
    GL_RGBA,        // format
    GL_FLOAT,       // type
    clearColor      // data
  );

  // WIP
  do_pixel_lighting_pass(camera_data, visibility_tree);

  do_geometry_pass(camera_data, gun_data);
  update_ssbos();
  do_ligh_culling_pass(camera_data);
  do_lighting_pass(camera_data.position);

  // DEBUG: count white pixels in megatex
  if (ImGui::Begin("Num visible pixels"))
  {
    static int count = 0;
    if (ImGui::Button("Recalculate"))
    {
      glBindTexture(GL_TEXTURE_2D, GraphicsSystem::get().megatex_handle);
      GLint w = 0, h = 0;
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

      std::vector<float> px(w * h * 4);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, px.data());

      count = 0;
      for (int i = 0; i < w * h; ++i)
        if (px[i*4] >= 1.f && px[i*4+1] >= 1.f && px[i*4+2] >= 1.f)
          ++count;
    }

    ImGui::Text("Number of visible pixels: %d",  count);
    ImGui::Text("Number of visible pixels: %dK", (int)ceil(count / 1000.0f));

    ImGui::Separator();

    if (ImGui::Button("Blit"))
    {
      auto& gfx = GraphicsSystem::get();

      GLint w = 0, h = 0;
      glBindTexture(GL_TEXTURE_2D, gfx.megatex_handle);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

      glCopyImageSubData(
        gfx.megatex_handle,       // src texture
        GL_TEXTURE_2D, 0,         // src target, mip level
        0, 0, 0,                  // src x, y, z
        gfx.megatex_input_handle, // dst texture
        GL_TEXTURE_2D, 0,         // dst target, mip level
        0, 0, 0,                  // dst x, y, z
        w, h, 1                   // width, height, depth
      );
    }

    if (ImGui::Button("Clear"))
    {
      const float zeros[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      glClearTexImage(
        GraphicsSystem::get().megatex_input_handle,
        0,        // mip level
        GL_RGBA,  // format
        GL_FLOAT, // type
        zeros
      );
    }
  }
  ImGui::End();

  if (ImGui::Begin("Pixel Light Megatex"))
  {
    auto& gfx = GraphicsSystem::get();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float aspect = (gfx.megatex_height > 0)
      ? (float)gfx.megatex_width / (float)gfx.megatex_height
      : 1.0f;
    ImVec2 img_size = { avail.x, avail.x / aspect };
    ImGui::Image((ImTextureID)(intptr_t)gfx.megatex_input_handle, img_size, ImVec2(0,1), ImVec2(1,0));
  }
  ImGui::End();


  m_dir_light_ssbo.clear();
  m_point_light_ssbo.clear();
  m_sector_matricies_ssbo.clear();

  m_light_gpu_data_indices.clear();
  m_sector_matricies.clear();

  m_light_checker.registry.clear();
  m_entity_checker.registry.clear();
}

//==============================================================================
const ShaderProgramHandle& Renderer::get_solid_material() const
{
  return m_solid_material;
}

//==============================================================================
void Renderer::update_sector_ssbos() const
{
  const MapSectors& map = GameSystem::get().get_map();

  m_sectors_ssbo.clear();
  m_walls_ssbo.clear();
  m_portal_matricies_ssbo.clear();

  m_portal_matricies_ssbo.push_back(mat4(1.0f));

  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
  {
    std::vector<WallGPU> walls_data;
    map.for_each_wall_of_sector(sector_id, [this, &map, &walls_data](WallID wall_id)
    {
      const WallData& wall = map.walls[wall_id];

      size_t matrix_index;
      if (wall.render_data_index == INVALID_SECTOR_ID)
      {
        // index of unit matrix
        matrix_index = 0;
      }
      else
      {
        const mat4& portal_matrix = map.portals_render_data[wall.render_data_index].dest_to_src;
        matrix_index = m_portal_matricies_ssbo.push_back(portal_matrix);
      }

      if (walls_data.size() > 0)
        walls_data[walls_data.size() - 1].end = wall.pos;

      walls_data.emplace_back(wall.pos, vec2(), 0, static_cast<u32>(wall.portal_sector_id), static_cast<u32>(matrix_index));
    });
    walls_data[walls_data.size() - 1].end = walls_data[0].start;

    for (WallGPU& wall_data : walls_data)
    {
      const vec2 direction = wall_data.end - wall_data.start;
      const vec2 normal = normalize(vec2(-direction.y, direction.x));
      const u32  packed_normal = packSnorm2x16(normal);

      wall_data.packed_normal = packed_normal;
    }

    const u32 walls_offset = m_walls_ssbo.size_u32();
    const u32 walls_count = static_cast<u32>(walls_data.size());
    m_walls_ssbo.extend(std::move(walls_data));

    m_sectors_ssbo.push_back
    (
      SectorGPU
      {
        .floor_y = map.sectors_dynamic[sector_id].floor_height,
        .ceil_y = map.sectors_dynamic[sector_id].ceil_height,
        .walls_offset = walls_offset,
        .walls_count = walls_count,
      }
    );
  }

  m_sectors_ssbo.update_gpu_data();
  m_walls_ssbo.update_gpu_data();
  m_portal_matricies_ssbo.update_gpu_data();
}

//==============================================================================
void Renderer::update_sector_heights(SectorID sector_id) const
{
  const MapSectors& map = GameSystem::get().get_map();
  const f32 heights[2] =
  {
    map.sectors_dynamic[sector_id].floor_height,
    map.sectors_dynamic[sector_id].ceil_height,
  };
  // Update only floor_y and ceil_y (first 8 bytes of SectorGPU)
  m_sectors_ssbo.update_gpu_item_bytes(sector_id, 0, heights, sizeof(heights));
}

//==============================================================================
std::pair<bool, size_t> Renderer::EntityRedundancyChecker::check_redundant(u64 id, vec3 pos)
{
  auto it = registry.find(id);
  if (it == registry.end())
  {
    // Never seen before, our first time together
    registry.insert({id, std::vector<vec3>{pos}});
    return { true, 0 };
  }

  std::vector<vec3>& tlist = it->second;
  for (size_t i = 0; i < tlist.size(); ++i)
  {
    const vec3& pt = tlist[i];

    if (distance2(pt, pos) <= DIST_THRESHOLD * DIST_THRESHOLD)
    {
      // It is there
      return { false, i };
    }
  }

  // We saw this light before, but from a different spot with a different
  // transformation matrix.
  tlist.push_back(pos);
  return { true, tlist.size() - 1 };
}

//==============================================================================
static GLuint create_g_buffer(GLint internal_format, GLenum attachment, u32 w, u32 h)
{
  GLenum format = GL_RGBA;
  GLenum type   = GL_FLOAT;

  // TODO: temp solution
  if (internal_format == GL_R32UI)
  {
    format = GL_RED_INTEGER;
    type   = GL_UNSIGNED_INT;
  }

  GLuint g_handle;
  glGenTextures(1, &g_handle);
  glBindTexture(GL_TEXTURE_2D, g_handle);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    internal_format,
    static_cast<GLsizei>(w),
    static_cast<GLsizei>(h),
    0,
    // format and type are not relevant for us because we modify the g-bufers only from GPU
    format,
    type,
    nullptr
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, g_handle, 0);

  return g_handle;
}

//==============================================================================
void Renderer::destroy_g_buffers()
{
  if (m_g_buffer)
  {
    nc_assert(glIsFramebuffer(m_g_buffer));
    glDeleteFramebuffers(1, &m_g_buffer);

    m_g_buffer = 0;
  }

  if (m_g_position)
  {
    // These must be valid always
    nc_assert(m_g_position);
    nc_assert(m_g_stitched_position);
    nc_assert(m_g_normal);
    nc_assert(m_g_stitched_normal);
    nc_assert(m_g_albedo);
    nc_assert(m_g_sector);
    nc_assert(glIsTexture(m_g_position));
    nc_assert(glIsTexture(m_g_stitched_position));
    nc_assert(glIsTexture(m_g_normal));
    nc_assert(glIsTexture(m_g_stitched_normal));
    nc_assert(glIsTexture(m_g_albedo));
    nc_assert(glIsTexture(m_g_sector));

    GLuint textures[6]{m_g_position, m_g_stitched_position, m_g_normal, m_g_stitched_normal, m_g_albedo, m_g_sector};
    glDeleteTextures(6, textures);

    m_g_position          = 0;
    m_g_stitched_position = 0;
    m_g_normal            = 0;
    m_g_stitched_normal   = 0;
    m_g_albedo            = 0;
    m_g_sector            = 0;
  }
}

//==============================================================================
void Renderer::create_g_buffers(u32 width, u32 height)
{
  glGenFramebuffers(1, &m_g_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_g_buffer);

  // create g buffers
  constexpr std::array<GLenum, 6> attachments =
  {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3,
    GL_COLOR_ATTACHMENT4,
    GL_COLOR_ATTACHMENT5,
  };
  // TODO: move specular strength to stitched normal g buffer
  // TODO: merge stitched normals and normals into one g buffer
  m_g_position          = create_g_buffer(GL_RGBA32F    , attachments[0], width, height);
  m_g_stitched_position = create_g_buffer(GL_RGBA32F    , attachments[1], width, height);
  m_g_normal            = create_g_buffer(GL_RGBA8_SNORM, attachments[2], width, height);
  m_g_stitched_normal   = create_g_buffer(GL_RGBA8_SNORM, attachments[3], width, height);
  m_g_albedo            = create_g_buffer(GL_RGBA8      , attachments[4], width, height);
  m_g_sector            = create_g_buffer(GL_R32UI      , attachments[5], width, height);
  glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());

  // create depth-stencil buffer
  GLuint depth_stencil_buffer;
  glGenRenderbuffers(1, &depth_stencil_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_buffer);
  glRenderbufferStorage(
    GL_RENDERBUFFER,
    GL_DEPTH24_STENCIL8,
    static_cast<GLsizei>(width),
    static_cast<GLsizei>(height)
  );
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_buffer);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    nc_warn("G-buffer not complete.");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//==============================================================================
void Renderer::recompute_projection(u32 width, u32 height, f32 fov)
{
  f32 aspect = static_cast<f32>(width) / height;

  // The near value has to be very tiny, because otherwise the camera clips into
  // the portals when traversing through them. On the other hand, making it too
  // small will result in losing a lot of depth buffer precision in the long
  // distance, which causes a flicering.
  //constexpr f32 NEAR = 0.001f;
  //constexpr f32 FAR  = 128.0f;
  m_default_projection = perspective(fov, aspect, NEAR, FAR);

  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::PROJECTION, m_default_projection);

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::PROJECTION, m_default_projection);

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::PROJECTION, m_default_projection);
}

//==============================================================================
void Renderer::do_geometry_pass
(
  const CameraData& camera, const RenderGunProperties& gun
)
const
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_g_buffer);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  
  const GLuint clear_value = 0;
  glClearBufferuiv(GL_COLOR, 5, &clear_value);

#ifdef NC_DEBUG_DRAW
  GizmoManager::get().draw_gizmos();
#endif

  render_sectors(camera);
  render_entities(camera);
  render_portals(camera);
  render_gun(camera, gun);
  render_sky_box(camera);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//==============================================================================
void Renderer::do_ligh_culling_pass(const CameraData& camera) const
{
  m_light_culling_shader.use();

  m_point_light_ssbo.bind(0);
  m_light_index_ssbo.bind(1);
  m_light_tiles_ssbo.bind(2);
  m_light_counter_ssbo.bind(3);

  m_light_counter_ssbo.clear();
  m_light_counter_ssbo.push_back(0);
  m_light_counter_ssbo.update_gpu_data();

  m_light_culling_shader.set_uniform(shaders::light_culling::VIEW, camera.view);
  m_light_culling_shader.set_uniform(shaders::light_culling::INV_PROJECTION, inverse(m_default_projection));
  m_light_culling_shader.set_uniform(shaders::light_culling::WINDOW_SIZE, cast<vec2>(m_window_size));
  m_light_culling_shader.set_uniform(shaders::light_culling::FAR_PLANE, FAR);
  m_light_culling_shader.set_uniform(shaders::light_culling::NUM_LIGHTS, m_point_light_ssbo.gpu_size_u32());

  const size_t num_tiles_x = (static_cast<size_t>(m_window_size.x) + LIGHT_CULLING_TILE_SIZE_X - 1)
    / LIGHT_CULLING_TILE_SIZE_X;
  const size_t num_tiles_y = (static_cast<size_t>(m_window_size.y) + LIGHT_CULLING_TILE_SIZE_Y - 1) 
    / LIGHT_CULLING_TILE_SIZE_Y;

  m_light_culling_shader.dispatch(num_tiles_x, num_tiles_y, 1, true);
}

//==============================================================================
void get_sectors(std::set<SectorID>& out, const VisibilityTree& tree)
{
  for (const auto[sid, _] : tree.sectors)
  {
    out.insert(sid);
  }

  for (const auto& subtree : tree.children)
  {
    get_sectors(out, subtree);
  }
}

//==============================================================================
void Renderer::do_pixel_lighting_pass
(
  [[maybe_unused]]const CameraData& camera,
  [[maybe_unused]]const VisibilityTree& tree
)
const
{
  auto& gfx = GraphicsSystem::get();
  const MapSectors& map = GameSystem::get().get_map();

  glPushDebugGroup
  (
    GL_DEBUG_SOURCE_APPLICATION,
    0,
    -1,
    "PixelLightingPass"
  );

  std::set<SectorID> sectors_to_render;
  get_sectors(sectors_to_render, tree);

  glBindFramebuffer(GL_FRAMEBUFFER, gfx.megatex_fbo);
  glViewport(0, 0, gfx.megatex_width, gfx.megatex_height);
  glClear(GL_COLOR_BUFFER_BIT);

  m_pixel_light_shader.use();

  const vec2 megatex_size = vec2(gfx.megatex_width, gfx.megatex_height);
  m_pixel_light_shader.set_uniform(shaders::pixel_light::MEGATEX_SIZE, megatex_size);
  m_pixel_light_shader.set_uniform(shaders::pixel_light::NUM_SECTORS,  m_sectors_ssbo.gpu_size_u32());
  m_pixel_light_shader.set_uniform(shaders::pixel_light::NUM_WALLS,    m_walls_ssbo.gpu_size_u32());

  EntityRegistry& registry = GameSystem::get().get_entities();
  registry.for_each<AmbientLight>([this](AmbientLight& ambient)
  {
    m_pixel_light_shader.set_uniform(shaders::pixel_light::AMBIENT_STRENGTH, ambient.strength);
  });

  m_sectors_ssbo.bind(1);
  m_walls_ssbo.bind(2);
  m_portal_matricies_ssbo.bind(3);
  m_sector_matricies_ssbo.bind(4);

  // Bind a dummy VAO — core profile requires one even when vertex data
  // comes entirely from gl_VertexID.
  const MeshHandle screen_quad = MeshManager::get().get_screen_quad();
  glBindVertexArray(screen_quad.get_vao());

  // Only floors
  for (SectorID sid : sectors_to_render)
  {
    MegatexPartId floor_id = map.sectors[sid].floor_megatex_id;
    MegatexPartId ceil_id  = map.sectors[sid].ceil_megatex_id;

    const auto& part_floor = gfx.megatex_parts[floor_id];
    const auto& part_ceil  = gfx.megatex_parts[ceil_id];

    m_pixel_light_shader.set_uniform(shaders::pixel_light::SECTOR_ID, cast<u32>(sid));

    // gather light info
    m_point_light_pixel_ssbo.clear();

    // Push back all lights
    const auto& mapping = GameSystem::get().get_sector_mapping();
    mapping.for_each_in_sector<PointLight>(sid, [&](EntityID id, mat4 t)
    {
      PointLight* light = GameSystem::get().get_entities().get_entity<PointLight>(id);
      nc_assert(light);

      vec3 pos = (t * vec4(light->get_position(), 1.0f)).xyz();
      SectorID light_sid = map.get_sector_from_point(light->get_position().xz());
      m_point_light_pixel_ssbo.push_back(light->get_gpu_data(pos, pos, light_sid));
    });

    // gpu data there
    m_point_light_pixel_ssbo.update_gpu_data();
    m_point_light_pixel_ssbo.bind(0);

    // set num lights
    m_pixel_light_shader.set_uniform(shaders::pixel_light::NUM_LIGHTS, m_point_light_pixel_ssbo.gpu_size_u32());

    // floor
    {
      m_pixel_light_shader.set_uniform(shaders::pixel_light::FROM,   cast<vec2>(part_floor.megatex_coord_1));
      m_pixel_light_shader.set_uniform(shaders::pixel_light::TO,     cast<vec2>(part_floor.megatex_coord_2));
      m_pixel_light_shader.set_uniform(shaders::pixel_light::COLOR,  vec3{1.0f, 0.0f, 0.0f});
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP00,   part_floor.wpos_00);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP10,   part_floor.wpos_10);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP01,   part_floor.wpos_01);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP11,   part_floor.wpos_11);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::NORMAL, part_floor.normal);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // ceil
    {
      m_pixel_light_shader.set_uniform(shaders::pixel_light::FROM,   cast<vec2>(part_ceil.megatex_coord_1));
      m_pixel_light_shader.set_uniform(shaders::pixel_light::TO,     cast<vec2>(part_ceil.megatex_coord_2));
      m_pixel_light_shader.set_uniform(shaders::pixel_light::COLOR,  vec3{0.0f, 0.0f, 1.0f});
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP00,   part_ceil.wpos_00);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP10,   part_ceil.wpos_10);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP01,   part_ceil.wpos_01);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::WP11,   part_ceil.wpos_11);
      m_pixel_light_shader.set_uniform(shaders::pixel_light::NORMAL, part_ceil.normal);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // all walls
    {
      map.for_each_wall_of_sector(sid, [&](WallID wid)
      {
        MegatexPartId wall_id = map.walls[wid].megatex_id;
        const auto&   part    = gfx.megatex_parts[wall_id];

        m_pixel_light_shader.set_uniform(shaders::pixel_light::FROM,   cast<vec2>(part.megatex_coord_1));
        m_pixel_light_shader.set_uniform(shaders::pixel_light::TO,     cast<vec2>(part.megatex_coord_2));
        m_pixel_light_shader.set_uniform(shaders::pixel_light::COLOR,  vec3{0.0f, 1.0f, 0.0f});
        m_pixel_light_shader.set_uniform(shaders::pixel_light::WP00,   part.wpos_00);
        m_pixel_light_shader.set_uniform(shaders::pixel_light::WP10,   part.wpos_10);
        m_pixel_light_shader.set_uniform(shaders::pixel_light::WP01,   part.wpos_01);
        m_pixel_light_shader.set_uniform(shaders::pixel_light::WP11,   part.wpos_11);
        m_pixel_light_shader.set_uniform(shaders::pixel_light::NORMAL, part.normal);
        glDrawArrays(GL_TRIANGLES, 0, 6);
      });
    }
  }

  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, m_window_size.x, m_window_size.y);

  glPopDebugGroup();
}

//==============================================================================
void Renderer::do_lighting_pass(const vec3& view_position) const
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // bind g-bufers
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_g_position);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_g_stitched_position);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_g_normal); glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, m_g_stitched_normal);
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, m_g_albedo);
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, m_g_sector);

  // bind light ssbos
  m_dir_light_ssbo.bind(0);
  m_point_light_ssbo.bind(1);
  m_light_index_ssbo.bind(2);
  m_light_tiles_ssbo.bind(3);
  m_sectors_ssbo.bind(4);
  m_walls_ssbo.bind(5);
  m_portal_matricies_ssbo.bind(6);
  m_sector_matricies_ssbo.bind(7);

  const size_t num_tiles_x = (static_cast<size_t>(m_window_size.x) + LIGHT_CULLING_TILE_SIZE_X - 1) 
    / LIGHT_CULLING_TILE_SIZE_X;

  // prepare shader
  m_light_material.use();
  m_light_material.set_uniform(shaders::light::VIEW_POSITION, view_position);
  m_light_material.set_uniform(shaders::light::NUM_DIR_LIGHTS, m_dir_light_ssbo.gpu_size_u32());
  m_light_material.set_uniform(shaders::light::NUM_TILES_X, static_cast<u32>(num_tiles_x));
  m_light_material.set_uniform(shaders::light::NUM_SECTORS, m_sectors_ssbo.gpu_size_u32());
  m_light_material.set_uniform(shaders::light::NUM_WALLS, m_walls_ssbo.gpu_size_u32());

  EntityRegistry& registry = GameSystem::get().get_entities();
  registry.for_each<AmbientLight>([this](AmbientLight& ambient)
  {
     m_light_material.set_uniform(shaders::light::AMBIENT_STRENGTH, ambient.strength);
  });

  // draw call
  const MeshHandle screen_quad = MeshManager::get().get_screen_quad();
  glBindVertexArray(screen_quad.get_vao());
  glDrawArrays(screen_quad.get_draw_mode(), 0, screen_quad.get_vertex_count());

  // clean up
  glBindVertexArray(0);
}

//==============================================================================
void Renderer::update_ssbos() const
{
  EntityRegistry& registry = GameSystem::get().get_entities();
  registry.for_each<DirectionalLight>([this](DirectionalLight& light)
  {
    m_dir_light_ssbo.push_back(light.get_gpu_data());
  });
  
  m_dir_light_ssbo.update_gpu_data();
  m_point_light_ssbo.update_gpu_data();
  m_sector_matricies_ssbo.update_gpu_data_with(m_sector_matricies);
}

//==============================================================================
void Renderer::render_sectors(const CameraData& camera) const
{
  auto& gfx = GraphicsSystem::get();
  const auto& sectors_to_render = camera.vis_tree.sectors;

  const auto& game_atlas = TextureManager::get().get_atlas(ResLifetime::Game);
  const auto& level_atlas = TextureManager::get().get_atlas(ResLifetime::Level);

  m_textures_ssbo.bind(0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, game_atlas.handle);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, level_atlas.handle);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, GraphicsSystem::get().megatex_input_handle);

  glBindImageTexture(
    7,              // binding unit
    GraphicsSystem::get().megatex_handle,            // texture id
    0,              // mip level
    GL_FALSE,       // layered
    0,              // layer
    GL_WRITE_ONLY,  // access
    GL_RGBA32F      // format
  );

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::VIEW, camera.view);
  m_sector_material.set_uniform(shaders::sector::PORTAL_DEST_TO_SRC, camera.portal_dest_to_src);

  for (const auto& [sector_id, _] : sectors_to_render)
  {
    const u32 matrix_id = static_cast<u32>(m_sector_matricies.size());
    m_sector_matricies.push_back(camera.portal_dest_to_src);

    m_sector_material.set_uniform(shaders::sector::SECTOR_ID, static_cast<u32>(sector_id));
    m_sector_material.set_uniform(shaders::sector::MATRIX_ID, matrix_id);

    // This returns the mesh and optionally updates it before if it is dirty.
    const MeshHandle& mesh = gfx.get_and_update_sector_mesh(sector_id);

    glBindVertexArray(mesh.get_vao());
    glDrawArrays(mesh.get_draw_mode(), 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//==============================================================================
// Chooses the exact texture handle for a given entity from the data in the
// appearance component.
static TextureHandle pick_texture_handle_from_appearance
(
  const Appearance&           appear,
  const Renderer::CameraData& camera
)
{
  switch (appear.mode)
  {
    // 1 directional sprite
    case Appearance::SpriteMode::mono:
    {
      return TextureManager::get()[appear.sprite];
      break;
    }

    // 8 directional sprite
    case Appearance::SpriteMode::dir8:
    {
      nc_assert(appear.direction != VEC3_ZERO, "Dir has to be filled out");

      vec3 dir_relative = camera.view * vec4{appear.direction, 0.0f};
      vec2 dir_2d = normalize_or_zero(vec2{dir_relative.x, dir_relative.z});

      if (dir_2d == VEC2_ZERO) [[unlikely]]
      {
        // Not sure if this can even happen legaly
        return TextureManager::get()[appear.sprite + "_d"];
      }

      // ( 0.0f, -1.0f) ->  0.0f   (looking straight at us)
      // ( 1.0f,  0.0f) ->  PI/2   (looking right)
      // (-1.0f,  0.0f) -> -PI/2   (looking left)
      // ( 0.0f,  1.0f) ->  PI     (looking back)
      f32 angle  = atan2f(dir_2d.x, dir_2d.y);
      f32 degree = rad2deg(angle);

      // 0 = right, 1 = left
      u8  side = degree < 0; // [0-1]
      // 0 = d, 1 = dl/dr, 2 = l/r, 3 = ul/ur/ 4 = u
      u8  face = cast<u8>((abs(degree) + 22.5f) / 45.0f); // [0-4]
      nc_assert(face < 5);

      // Now calculate the suffix from those 10 combinations
      u8 idx = side * 5 + face;
      nc_assert(idx < 10);

      constexpr cstr SUFFIX_LUT[]
      {
        // RIGHT
        "_d",  // 0 + 0 = 0
        "_dr", // 0 + 1 = 1
        "_r",  // 0 + 2 = 2
        "_ur", // 0 + 3 = 3
        "_u",  // 0 + 4 = 4
        // LEFT
        "_d",  // 5 + 0 = 5
        "_dl", // 5 + 1 = 6
        "_l",  // 5 + 2 = 7
        "_ul", // 5 + 3 = 8
        "_u",  // 5 + 4 = 9
      };

      return TextureManager::get()[appear.sprite + SUFFIX_LUT[idx]];
      break;
    }

    default:
    {
      nc_assert(false, "This should not happen!");
      return TextureHandle::invalid();
    }
  }
}

//==============================================================================
static mat4 calc_billboard_rotation
(
  const Renderer::CameraData& camera, vec3 position, bool with_camera, bool only_hor
)
{
  if (with_camera)
  {
    // Extracting X and Y components from the forward vector.
    mat3 camera_rotation = transpose(mat3(camera.view));

    if (only_hor)
    {
      return eulerAngleY(atan2(-camera_rotation[2][0], -camera_rotation[2][2]));
    }
    else
    {
      return mat4
      {
        vec4{-camera_rotation[0], 0},
        vec4{camera_rotation[1],  0},
        vec4{-camera_rotation[2], 0},
        vec4{0, 0, 0, 1}
      };
    }
  }
  else
  {
    vec3 camera_position_rel = (inverse(camera.view) * vec4{VEC3_ZERO, 1.0f}).xyz();

    vec3 to_camera_dir   = normalize_or(position - camera_position_rel, VEC3_X);
    vec2 to_camera_2     = normalize_or(to_camera_dir.xz(), VEC2_X);
    vec3 to_camera_hor   = vec3{to_camera_2.x, 0.0f, to_camera_2.y};
    vec3 to_camera_right = vec3{to_camera_2.y, 0.0f, -to_camera_2.x};
    vec3 to_camera_up    = UP_DIR;

    mat3 camera_rotation = transpose(mat3(camera.view));

    if (only_hor)
    {
      return mat4
      {
        vec4{to_camera_right, 0.0f},
        vec4{to_camera_up,    0.0f},
        vec4{to_camera_hor,   0.0f},
        vec4{VEC3_ZERO,       1.0f}
      };
    }
    else
    {
      return mat4
      {
        vec4{-camera_rotation[0], 0},
        vec4{camera_rotation[1],  0},
        vec4{-camera_rotation[2], 0},
        vec4{0, 0, 0, 1}
      };
    }
  }
}

//==============================================================================
void Renderer::render_entities(const CameraData& camera) const
{
  constexpr f32 BILLBOARD_TEXTURE_SCALE = 1.0f / 2048.0f;

  // group entities by texture atlas
  const auto& mapping = GameSystem::get().get_sector_mapping();
  const EntityRegistry& registry = GameSystem::get().get_entities();

  struct EntityRenderData
  {
    Appearance        appear;
    vec3              world_pos;
    std::vector<mat4> transforms;
    u32               sector_id;
    u32               matrix_id;
  };

  std::unordered_map<u64, EntityRenderData> groups[3]{};

  for (const auto& frustum : camera.vis_tree.sectors)
  {
    const SectorID sector_id = frustum.sector;

    mapping.for_each_in_sector(sector_id, [&](EntityID id, mat4 t)
    {
      const Entity* entity = registry.get_entity(id);
      nc_assert(entity);
      vec3 world_pos  = entity->get_position();
      vec3 camera_pos = (camera.view * t * vec4{world_pos, 1.0f}).xyz();

      const SectorID entity_sector_id = GameSystem::get().get_map().get_sector_from_point(world_pos.xz());

      if (id.type == EntityTypes::point_light)
      {
        // Now, we have to make sure to not include any light twice..
        // However, this is not an easy task, as one light can shine on us from
        // two different portals at the same time. Therefore, we have to first
        // distinguish them by their ID and then by their position.
        auto [is_unique, index] = m_light_checker.check_redundant(id.as_u32(), camera_pos);
        if (is_unique)
        {
          // Has to exist because it is in sector mapping
          nc_assert(registry.get_entity(id));

          const PointLight* light = registry.get_entity(id)->as<PointLight>();
          // Has to be a light because we would not get here otherwise
          nc_assert(light); 

          const vec3 light_pos = t * vec4{ world_pos, 1.0f };
          const vec3 stich_pos = camera.portal_dest_to_src * vec4(light_pos, 1.0f);

          const size_t light_gpu_index = m_point_light_ssbo.push_back
          (
            light->get_gpu_data(light_pos, stich_pos, entity_sector_id)
          );

          m_light_gpu_data_indices.emplace
          (
            (static_cast<u64>(id.as_u32()) << 32) + index,
            light_gpu_index
          );
        }
      }
      else
      {
        const Appearance* appearance = entity->get_appearance();
        if (!appearance)
        {
          return;
        }

        NC_TODO("Add multiple lifetimes to rendering of entities.");
        //const ResLifetime lifetime = texture.get_lifetime();
        //groups[lifetime].insert(entity);

				// TODO: This causes enemies to disappear in certain situations.
        //if (m_entity_checker.check_redundant(id.as_u32(), camera_pos).first)
        if (true)
        {
          auto& group = groups[cast<u64>(ResLifetime::Game)];
          EntityRenderData& render_data = group[id.as_u32()];
          const u32 matrix_id = static_cast<u32>(m_sector_matricies.size());
          m_sector_matricies.push_back(camera.portal_dest_to_src);

          render_data.appear    = *appearance;
          render_data.world_pos = world_pos;
          render_data.sector_id = static_cast<u32>(entity_sector_id);
          render_data.matrix_id = matrix_id;
          render_data.transforms.push_back(t);
        }
      }
    });
  }

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::VIEW, camera.view);
  m_billboard_material.set_uniform(shaders::billboard::PORTAL_DEST_TO_SRC, camera.portal_dest_to_src);

  const MeshHandle& texturable_quad = MeshManager::get().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());
  glActiveTexture(GL_TEXTURE0);

  for (u64 l = cast<u64>(ResLifetime::Level); l <= cast<u64>(ResLifetime::Game); ++l)
  {
    const auto& group = groups[l];
    const TextureAtlas& atlas = TextureManager::get().get_atlas(cast<ResLifetime>(l));
    glBindTexture(GL_TEXTURE_2D, atlas.handle);
    m_billboard_material.set_uniform(shaders::billboard::ATLAS_SIZE, atlas.get_size());
    m_billboard_material.set_uniform(shaders::billboard::ENABLE_SHADOWS, false);

    for (const auto& [entity, render_data] : group)
    {
      const Appearance& base_appearance = render_data.appear;
      const vec3 base_position = render_data.world_pos;

      m_billboard_material.set_uniform(shaders::billboard::SECTOR_ID, render_data.sector_id);
      m_billboard_material.set_uniform(shaders::billboard::MATRIX_ID, render_data.matrix_id);

      for (const mat4& entity_transform : render_data.transforms)
      {
        Appearance appearance = base_appearance;
        appearance.direction = (entity_transform * vec4{appearance.direction, 0.0f}).xyz();
        vec3 position = (entity_transform * vec4{base_position, 1.0f}).xyz();

        const TextureHandle& texture = pick_texture_handle_from_appearance
        (
          appearance, camera
        );

        m_billboard_material.set_uniform(shaders::billboard::TEXTURE_POS, texture.get_pos());
        m_billboard_material.set_uniform(shaders::billboard::TEXTURE_SIZE, texture.get_size());

        const bool bottom_mode = appearance.pivot == Appearance::PivotMode::bottom;
        const vec3 root_offset = bottom_mode ? VEC3_Y * 0.5f : VEC3_ZERO;

        const bool rot_only_hor = appearance.rotation == Appearance::RotationMode::only_horizontal;
        const mat4 rotation = calc_billboard_rotation
        (
          camera, position, CVars::billboard_cam_rot, rot_only_hor
        );

        const mat4 offset_transform = translation(root_offset);
        const f32  height_move = bottom_mode ? 0.0f : (BILLBOARD_TEXTURE_SCALE * 0.5f);

        const bool tex_size_scaling = appearance.scaling == Appearance::ScalingMode::texture_size;
        const f32 t_height = tex_size_scaling ? texture.get_height() : 64.0f;
        const f32 t_width  = tex_size_scaling ? texture.get_width()  : 64.0f;

        const vec3 pivot_offset(0.0f, t_height * height_move, 0.0f);
        const vec3 scale(
          t_width  * appearance.scale * BILLBOARD_TEXTURE_SCALE,
          t_height * appearance.scale * BILLBOARD_TEXTURE_SCALE,
          1.0f
        );

        const mat4 transform = translation(position + pivot_offset)
          * rotation
          * scaling(scale)
          * offset_transform;

        m_billboard_material.set_uniform(shaders::billboard::TRANSFORM, transform); 
        glDrawArrays(texturable_quad.get_draw_mode(), 0, texturable_quad.get_vertex_count());
      }
    }
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

//==============================================================================
void Renderer::render_portals(const CameraData& camera) const
{
  const MapSectors& map = get_engine().get_map();

  glEnable(GL_STENCIL_TEST);

  for (const auto& subtree : camera.vis_tree.children)
  {
    const auto portal_id = subtree.portal_wall;
    const auto& wall = map.walls[portal_id];
    nc_assert(wall.get_portal_type() == PortalType::non_euclidean);
    nc_assert(wall.render_data_index != INVALID_PORTAL_RENDER_ID);
    const Portal& render_data = map.portals_render_data[wall.render_data_index];

    const CameraData new_camera
    {
      .position   = camera.position,
      .view       = camera.view,
      .vis_tree   = subtree,
      .portal_dest_to_src = mat4(1.0f),
      .portal_id = subtree.portal_wall,
    };

    render_portal(new_camera, render_data, 0);
  }

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_STENCIL_TEST);
}

//==============================================================================
void Renderer::render_gun
(
  const CameraData& camera, const RenderGunProperties& gun
)
const
{
  namespace sb = shaders::billboard;

  if (gun.sprite.empty())
  {
    // Early exit if no gun should be rendered
    return;
  }

  const TextureHandle& texture         = TextureManager::get()[gun.sprite];
  const MeshHandle&    texturable_quad = MeshManager::get().get_texturable_quad();

  f32  gun_zoom = CVars::gun_zoom;
  vec2 win_size = cast<vec2>(m_window_size);
  vec3 scale    = -vec3{win_size.x * gun_zoom, win_size.y * gun_zoom, 1.0f};
  f32  trans_x  = 0.5f + gun.sway.x * 0.5f;
  f32  trans_y  = 0.5f + gun.sway.y * 0.5f;
  vec3 trans    = vec3{trans_x * win_size.x, trans_y * win_size.y, 0.0f};

  mat4 transform  = inverse(camera.view);
  mat4 view       = translation(trans) * scaling(scale);
  mat4 projection = ortho(0.0f, win_size.x, win_size.y, 0.0f, -1.0f, 1.0f);

  const vec2 player_position = ((Entity*)GameHelpers::get().get_player())->get_position().xz;
  const SectorID sector_id = GameSystem::get().get_map().get_sector_from_point(player_position);

  const u32 matrix_id = static_cast<u32>(m_sector_matricies.size());
  m_sector_matricies.push_back(camera.portal_dest_to_src);

  glBindVertexArray(texturable_quad.get_vao());

  m_gun_material.use();
  m_gun_material.set_uniform(sb::TRANSFORM,      transform);
  m_gun_material.set_uniform(sb::VIEW,           view);
  m_gun_material.set_uniform(sb::PROJECTION,     projection);
  m_gun_material.set_uniform(sb::ATLAS_SIZE,     texture.get_atlas().get_size());
  m_gun_material.set_uniform(sb::TEXTURE_POS,    texture.get_pos());
  m_gun_material.set_uniform(sb::TEXTURE_SIZE,   texture.get_size());
  m_gun_material.set_uniform(sb::SECTOR_ID,      static_cast<u32>(sector_id));
  m_gun_material.set_uniform(sb::MATRIX_ID,      matrix_id);
  m_gun_material.set_uniform(sb::ENABLE_SHADOWS, true);

  glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
  glDrawArrays(texturable_quad.get_draw_mode(), 0, texturable_quad.get_vertex_count());

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

void Renderer::render_sky_box(const CameraData& camera) const
{
  EntityRegistry& registry = GameSystem::get().get_entities();
  const MeshHandle& cube = MeshManager::get().get_cube();

  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);
  glCullFace(GL_FRONT);

  m_sky_box_material.use();
  m_sky_box_material.set_uniform(shaders::sky_box::VIEW, camera.view);
  m_sky_box_material.set_uniform(shaders::sky_box::PROJECTION, m_default_projection);

  glActiveTexture(GL_TEXTURE0);
  registry.for_each<SkyBox>([this](const SkyBox& sky_box)
  {
    glBindTexture(GL_TEXTURE_2D, sky_box.get_texture_handle());

    m_sky_box_material.set_uniform(shaders::sky_box::EXPOSURE, sky_box.exposure);
    m_sky_box_material.set_uniform(shaders::sky_box::USE_GAMMA_CORRECTION, sky_box.use_gamma_correction);
  });

  glBindVertexArray(cube.get_vao());
  glDrawArrays(cube.get_draw_mode(), 0, cube.get_vertex_count());

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
}

#pragma region portals rendering
//==============================================================================
void Renderer::render_portal_to_stencil(const CameraData& camera, const Portal& portal, u8 recursion) const
{
  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::UNLIT, true);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera.view);
  m_solid_material.set_uniform(shaders::solid::TRANSFORM, portal.transform);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  glStencilFunc(GL_LEQUAL, recursion, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

  const MeshHandle& quad = MeshManager::get().get_quad();
  glBindVertexArray(quad.get_vao());
  glDrawArrays(quad.get_draw_mode(), 0, quad.get_vertex_count());
  glBindVertexArray(0);
}

//==============================================================================
void Renderer::render_portal_to_color(const CameraData& camera, u8 recursion) const
{
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilFunc(GL_LEQUAL, recursion + 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  render_sectors(camera);
  render_entities(camera);
}

//==============================================================================
void Renderer::render_portal_to_depth(
  const CameraData& camera,
  const Portal&     portal,
  bool              depth_write,
  u8                recursion
) const
{
  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::UNLIT, true);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera.view);
  m_solid_material.set_uniform(shaders::solid::TRANSFORM, portal.transform);
  m_solid_material.set_uniform(shaders::solid::COLOR, colors::LIME);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthFunc(GL_ALWAYS);

  if (depth_write)
  {
    // MR says: Here, we render the portal into the depth buffer, but
    //          set all pixel depths to 1.0 (maximum one) - this resets
    //          the depth so that recurisive rendering of the portal
    //          does not produce artifacts.
    // Overwrite the depth value to maximum one
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    // This makes sure that the final depth will be 1.0 everywhere
    glDepthRange(1.0, 1.0);
    // No need to reset this later, a it will be changed by the
    // "render_portal_to_color_function"
    glDepthMask(GL_TRUE);
    // This makes sure that we write depth 1.0 on the whole surface
    // of the portal
    glStencilFunc(GL_LEQUAL, recursion + 1, 0xFF);
  }
  else
  {
    // Render the portal in a normal way
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
  }

  const MeshHandle& quad = MeshManager::get().get_quad();
  glBindVertexArray(quad.get_vao());
  glDrawArrays(quad.get_draw_mode(), 0, quad.get_vertex_count());
  glBindVertexArray(0);

  // Reset the depth function back
  glDepthFunc(GL_LESS);

  if (depth_write)
  {
    glDepthRange(0.0, 1.0); // Back to normal
  }
}

//==============================================================================
void Renderer::render_portal(const CameraData& camera, const Portal& portal, u8 recursion) const
{
  const MapSectors& map = get_engine().get_map();
  const mat4 virtual_view = camera.view * portal.dest_to_src;
  [[maybe_unused]] const vec3 virtual_view_pos = vec3(inverse(virtual_view) * vec4(0.0f, 0.0f, 0.0f, 1.0f));

  const CameraData virtual_camera_data = CameraData
  {
    .position = camera.position,
    .view = virtual_view,
    .vis_tree = camera.vis_tree,
    .portal_dest_to_src = camera.portal_dest_to_src * portal.dest_to_src,
    .portal_id = camera.portal_id,
  };

  render_portal_to_stencil(camera, portal, recursion);
  render_portal_to_depth(camera, portal, true, recursion);
  render_portal_to_color(virtual_camera_data, recursion);

  for (const auto& subtree : camera.vis_tree.children)
  {
    const WallData& wall = map.walls[subtree.portal_wall];
    nc_assert(wall.get_portal_type() == PortalType::non_euclidean);

    const Portal& render_data = map.portals_render_data[wall.render_data_index];
    const CameraData new_cam_data
    {
      .position   = virtual_camera_data.position,
      .view       = virtual_camera_data.view,
      .vis_tree   = subtree,
      .portal_dest_to_src = virtual_camera_data.portal_dest_to_src,
      .portal_id = subtree.portal_wall,
    };

    render_portal(new_cam_data, render_data, recursion + 1);
  }
  glStencilFunc(GL_LEQUAL, recursion + 1, 0xFF);

  render_portal_to_depth(camera, portal, false, recursion);
}
#pragma endregion

}