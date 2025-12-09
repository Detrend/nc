// Project Nucledian Source File
#include <engine/graphics/renderer.h>

#include <common.h>
#include <logging.h>
#include <cvars.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/graphics/camera.h>
#include <engine/graphics/gizmo.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/entities/lights.h>
#include <engine/graphics/entities/sky_box.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/core/engine.h>
#include <engine/map/map_system.h>
#include <engine/player/game_system.h>
#include <engine/appearance.h>

#include <array>
#include <unordered_set>

namespace nc
{

//==============================================================================
Renderer::Renderer(u32 win_w, u32 win_h)
: m_solid_material(shaders::solid::VERTEX_SOURCE, shaders::solid::FRAGMENT_SOURCE)
, m_billboard_material(shaders::billboard::VERTEX_SOURCE, shaders::billboard::FRAGMENT_SOURCE)
, m_gun_material(shaders::gun::VERTEX_SOURCE, shaders::gun::FRAGMENT_SOURCE)
, m_light_material(shaders::light::VERTEX_SOURCE, shaders::light::FRAGMENT_SOURCE)
, m_sector_material(shaders::sector::VERTEX_SOURCE, shaders::sector::FRAGMENT_SOURCE)
, m_light_culling_shader(shaders::light_culling::COMPUTE_SOURCE)
, m_sky_box_material(shaders::sky_box::VERTEX_SOURCE, shaders::sky_box::FRAGMENT_SOURCE)
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
  struct TextureData
  {
    vec2 pos;
    vec2 size;
    f32  in_game_atlas;
    f32  _padding;
  };
  glGenBuffers(1, &m_texture_data_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_texture_data_ssbo);
  const std::vector<TextureHandle> textures = TextureManager::get().get_textures();
  std::vector<TextureData> data(textures.size());
  for (std::size_t i = 0; i < data.size(); ++i)
  {
    data[i].pos = textures[i].get_pos();
    data[i].size = textures[i].get_size();
    data[i].in_game_atlas = (textures[i].get_lifetime() == ResLifetime::Game ? 1.0f : 0.0f);
  }
  glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(TextureData), data.data(), GL_STATIC_DRAW);

  // setup light culling and light pass buffers
  
  const size_t num_tiles_x = (win_w + LIGHT_CULLING_TILE_SIZE_X - 1) / LIGHT_CULLING_TILE_SIZE_X;
  const size_t num_tiles_y = (win_h + LIGHT_CULLING_TILE_SIZE_Y - 1) / LIGHT_CULLING_TILE_SIZE_Y;
  const size_t num_tiles = num_tiles_x * num_tiles_y;

  std::array<std::pair<GLuint*, size_t>, 5> buffers
  {{
    { &m_dir_light_ssbo, DirectionalLight::MAX_DIRECTIONAL_LIGHTS * sizeof(DirLightGPU) },
    { &m_point_light_ssbo, PointLight::MAX_VISIBLE_POINT_LIGHTS * sizeof(PointLightGPU) },
    { &m_light_index_ssbo, PointLight::MAX_LIGHTS_PER_TILE * num_tiles * sizeof(u32) },
    { &m_tile_data_ssbo, num_tiles * 2 * sizeof(u32) },
    { &m_atomic_counter, sizeof(u32) },
  }};

  for (auto&& [buffer, size] : buffers)
  {
    glGenBuffers(1, buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//==============================================================================
void Renderer::on_window_resized(u32 w, u32 h)
{
  m_window_size = ivec2{w, h};

  // clean them up first before recreating
  this->destroy_g_buffers();

  // and recreate with the new size
  this->create_g_buffers(w, h);

  // and recompute projection matrix for this aspect ratio
  this->recompute_projection(w, h, GraphicsSystem::FOV);

  // resize ssbo buffers
  const size_t num_tiles_x = (w + LIGHT_CULLING_TILE_SIZE_X - 1) / LIGHT_CULLING_TILE_SIZE_X;
  const size_t num_tiles_y = (h + LIGHT_CULLING_TILE_SIZE_Y - 1) / LIGHT_CULLING_TILE_SIZE_Y;
  const size_t num_tiles = num_tiles_x * num_tiles_y;

  std::array<std::pair<GLuint*, size_t>, 2> buffers
  {{
    { &m_light_index_ssbo, PointLight::MAX_LIGHTS_PER_TILE * num_tiles * sizeof(u32) },
    { &m_tile_data_ssbo, num_tiles * 2 * sizeof(u32) },
  }};

  for (auto&& [buffer, size] : buffers)
  {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//==============================================================================
void Renderer::render
(
  const VisibilityTree& visibility_tree,
  const RenderGunProperties& gun_data
)
const
{
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

  do_geometry_pass(camera_data, gun_data);
  do_ligh_culling_pass(camera_data);
  do_lighting_pass(camera_data.position);

  m_light_checker.registry.clear();
  m_entity_checker.registry.clear();
}

//==============================================================================
const ShaderProgramHandle& Renderer::get_solid_material() const
{
  return m_solid_material;
}

//==============================================================================
bool Renderer::EntityRedundancyChecker::check_redundant(u64 id, vec3 pos)
{
  auto it = registry.find(id);
  if (it == registry.end())
  {
    // Never seen before, our first time together
    registry.insert({id, std::vector<vec3>{pos}});
    return true;
  }

  std::vector<vec3>& tlist = it->second;
  for (vec3 pt : tlist)
  {
    if (distance2(pt, pos) <= DIST_THRESHOLD * DIST_THRESHOLD)
    {
      // It is there
      return false;
    }
  }

  // We saw this light before, but from a different spot with a different
  // transformation matrix.
  tlist.push_back(pos);
  return true;
}

//==============================================================================
static GLuint create_g_buffer(GLint internal_format, GLenum attachment, u32 w, u32 h)
{
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
    GL_RGBA,
    GL_FLOAT,
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
    nc_assert(m_g_normal);
    nc_assert(m_g_albedo);
    nc_assert(glIsTexture(m_g_position));
    nc_assert(glIsTexture(m_g_normal));
    nc_assert(glIsTexture(m_g_albedo));

    GLuint textures[3]{m_g_position, m_g_normal, m_g_albedo};
    glDeleteTextures(3, textures);

    m_g_position = 0;
    m_g_normal   = 0;
    m_g_albedo   = 0;
  }
}

//==============================================================================
void Renderer::create_g_buffers(u32 width, u32 height)
{
  glGenFramebuffers(1, &m_g_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_g_buffer);

  // create g buffers
  constexpr std::array<GLenum, 4> attachments =
  {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3
  };
  // TODO: move specular strength to stitched normal g buffer
  m_g_position        = create_g_buffer(GL_RGBA32F    , attachments[0], width, height);
  // TODO: merge stitched normals and normals into one g buffer
  m_g_normal          = create_g_buffer(GL_RGBA8_SNORM, attachments[1], width, height);
  m_g_stitched_normal = create_g_buffer(GL_RGB8_SNORM, attachments[2], width, height);
  m_g_albedo          = create_g_buffer(GL_RGBA8      , attachments[3], width, height);
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
  update_light_ssbos();

  m_light_culling_shader.use();

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_point_light_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_light_index_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_tile_data_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_atomic_counter);

  u32 zero = 0;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_atomic_counter);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(u32), &zero);

  m_light_culling_shader.set_uniform(shaders::light_culling::VIEW, camera.view);
  m_light_culling_shader.set_uniform(shaders::light_culling::INV_PROJECTION, inverse(m_default_projection));
  m_light_culling_shader.set_uniform(shaders::light_culling::WINDOW_SIZE, cast<vec2>(m_window_size));
  m_light_culling_shader.set_uniform(shaders::light_culling::FAR_PLANE, FAR);
  m_light_culling_shader.set_uniform(shaders::light_culling::NUM_LIGHTS, m_point_light_ssbo_size);

  const size_t num_tiles_x = (static_cast<size_t>(m_window_size.x) + LIGHT_CULLING_TILE_SIZE_X - 1)
    / LIGHT_CULLING_TILE_SIZE_X;
  const size_t num_tiles_y = (static_cast<size_t>(m_window_size.y) + LIGHT_CULLING_TILE_SIZE_Y - 1) 
    / LIGHT_CULLING_TILE_SIZE_Y;

  m_light_culling_shader.dispatch(num_tiles_x, num_tiles_y, 1, true);

  // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//==============================================================================
void Renderer::do_lighting_pass(const vec3& view_position) const
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // bind g-bufers
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_g_position);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_g_normal);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_g_stitched_normal);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, m_g_albedo);

  // bind light ssbos
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_dir_light_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_point_light_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_light_index_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_tile_data_ssbo);

  const size_t num_tiles_x = (static_cast<size_t>(m_window_size.x) + LIGHT_CULLING_TILE_SIZE_X - 1) 
    / LIGHT_CULLING_TILE_SIZE_X;

  // prepare shader
  m_light_material.use();
  m_light_material.set_uniform(shaders::light::VIEW_POSITION, view_position);
  m_light_material.set_uniform(shaders::light::NUM_DIR_LIGHTS, m_dir_light_ssbo_size);
  m_light_material.set_uniform(shaders::light::NUM_TILES_X, static_cast<u32>(num_tiles_x));

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
void Renderer::update_light_ssbos() const
{
  // get all directional lights
  std::vector<DirLightGPU> dir_light_data;
  EntityRegistry& registry = GameSystem::get().get_entities();
  registry.for_each<DirectionalLight>([&dir_light_data](DirectionalLight& light)
  {
    dir_light_data.push_back(light.get_gpu_data());
  });
  m_dir_light_ssbo_size = static_cast<u32>(dir_light_data.size());

  // update directional light ssbo
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_dir_light_ssbo);
  glBufferSubData
  (
    GL_SHADER_STORAGE_BUFFER,
    0,
    dir_light_data.size() * sizeof(DirLightGPU),
    dir_light_data.data()
  );

  // update point lights ssbo
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_point_light_ssbo);
  glBufferSubData
  (
    GL_SHADER_STORAGE_BUFFER,
    0,
    m_point_light_data.size() * sizeof(PointLightGPU),
    m_point_light_data.data()
  );
  m_point_light_ssbo_size = static_cast<u32>(m_point_light_data.size());

  // clean up
  m_point_light_data.clear();
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//==============================================================================
void Renderer::render_sectors(const CameraData& camera) const
{
  auto& gfx = GraphicsSystem::get();
  const auto& sectors_to_render = camera.vis_tree.sectors;

  const auto& game_atlas = TextureManager::get().get_atlas(ResLifetime::Game);
  const auto& level_atlas = TextureManager::get().get_atlas(ResLifetime::Level);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_texture_data_ssbo);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, game_atlas.handle);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, level_atlas.handle);

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::VIEW, camera.view);
  m_sector_material.set_uniform(shaders::sector::PORTAL_DEST_TO_SRC, camera.portal_dest_to_src);

  for (const auto& [sector_id, _] : sectors_to_render)
  {
    // This returns the mesh and optionally updates it before if it is dirty.
    const MeshHandle& mesh = gfx.get_and_update_sector_mesh(sector_id);

    glBindVertexArray(mesh.get_vao());
    glDrawArrays(mesh.get_draw_mode(), 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
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
  };

  std::unordered_map<u64, EntityRenderData> groups[3]{};

  for (const auto& frustum : camera.vis_tree.sectors)
  {
    SectorID sid = frustum.sector;
    mapping.for_each_in_sector(sid, [&](EntityID id, mat4 t)
    {
      const Entity* entity = registry.get_entity(id);
      nc_assert(entity);
      vec3 world_pos  = entity->get_position();
      vec3 camera_pos = (camera.view * t * vec4{world_pos, 1.0f}).xyz();

      if (id.type == EntityTypes::point_light)
      {
        // Now, we have to make sure to not include any light twice..
        // However, this is not an easy task, as one light can shine on us from
        // two different portals at the same time. Therefore, we have to first
        // distinguish them by their ID and then by their position.
        if (m_light_checker.check_redundant(id.as_u64(), camera_pos))
        {
          // Has to exist because it is in sector mapping
          nc_assert(registry.get_entity(id));

          const PointLight* light = registry.get_entity(id)->as<PointLight>();

          // Has to be a light because we would not get here otherwise
          nc_assert(light); 

          vec3 light_pos = t * vec4{light->get_position(), 1.0f};
          vec3 stich_pos = camera.portal_dest_to_src * vec4(light_pos, 1.0f);
          m_point_light_data.push_back(light->get_gpu_data(stich_pos));
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

        if (m_entity_checker.check_redundant(id.as_u64(), camera_pos))
        {
          auto& group = groups[cast<u64>(ResLifetime::Game)];
          EntityRenderData& render_data = group[id.as_u64()];
          render_data.appear    = *appearance;
          render_data.world_pos = entity->get_position();
          render_data.transforms.push_back(t);
        }
      }
    });
  }

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::VIEW, camera.view);

  const MeshHandle& texturable_quad = MeshManager::get().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());
  glActiveTexture(GL_TEXTURE0);

  const mat3 camera_rotation = transpose(mat3(camera.view));
  // Extracting X and Y components from the forward vector.
  const float yaw = atan2(-camera_rotation[2][0], -camera_rotation[2][2]);
  const mat4 rotation_horizontal = eulerAngleY(yaw);
  const mat4 rotation_full = mat4
  {
    vec4{-camera_rotation[0], 0},
    vec4{camera_rotation[1],  0},
    vec4{-camera_rotation[2], 0},
    vec4{0, 0, 0, 1}
  };

  for (u64 l = cast<u64>(ResLifetime::Level); l <= cast<u64>(ResLifetime::Game); ++l)
  {
    const auto& group = groups[l];
    const TextureAtlas& atlas = TextureManager::get().get_atlas(cast<ResLifetime>(l));
    glBindTexture(GL_TEXTURE_2D, atlas.handle);
    m_billboard_material.set_uniform(shaders::billboard::ATLAS_SIZE, atlas.get_size());

    for (const auto& [entity, render_data] : group)
    {
      const Appearance& base_appearance = render_data.appear;
      const vec3 base_position = render_data.world_pos;

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
        const mat4& rotation = rot_only_hor ? rotation_horizontal : rotation_full;

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

  glBindVertexArray(texturable_quad.get_vao());

  m_gun_material.use();
  m_gun_material.set_uniform(sb::TRANSFORM,    transform);
  m_gun_material.set_uniform(sb::VIEW,         view);
  m_gun_material.set_uniform(sb::PROJECTION,   projection);
  m_gun_material.set_uniform(sb::ATLAS_SIZE,   texture.get_atlas().get_size());
  m_gun_material.set_uniform(sb::TEXTURE_POS,  texture.get_pos());
  m_gun_material.set_uniform(sb::TEXTURE_SIZE, texture.get_size());

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
    };

    render_portal(new_cam_data, render_data, recursion + 1);
  }
  glStencilFunc(GL_LEQUAL, recursion + 1, 0xFF);

  render_portal_to_depth(camera, portal, false, recursion);
}
#pragma endregion

}