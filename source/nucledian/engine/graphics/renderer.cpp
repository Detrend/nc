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
#include <engine/graphics/lights.h>
#include <engine/graphics/shaders/shaders.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/core/engine.h>
#include <engine/map/map_system.h>
#include <engine/player/thing_system.h>
#include <engine/appearance.h>

#include <unordered_set>

namespace nc
{

//==============================================================================
Renderer::Renderer(u32 win_w, u32 win_h)
: m_solid_material(shaders::solid::VERTEX_SOURCE, shaders::solid::FRAGMENT_SOURCE)
, m_billboard_material(shaders::billboard::VERTEX_SOURCE, shaders::billboard::FRAGMENT_SOURCE)
, m_light_material(shaders::light::VERTEX_SOURCE, shaders::light::FRAGMENT_SOURCE)
, m_sector_material(shaders::sector::VERTEX_SOURCE, shaders::sector::FRAGMENT_SOURCE)
{
  this->create_g_buffers(win_w, win_h);
  this->recompute_projection(win_w, win_h, GraphicsSystem::FOV);

  const vec2 game_atlas_size = TextureManager::get().get_atlas(ResLifetime::Game).get_size();
  const vec2 level_atlas_size = TextureManager::get().get_atlas(ResLifetime::Level).get_size();

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::TEXTURE, 0);

  m_light_material.use();
  m_light_material.set_uniform(shaders::light::G_POSITION, 0);
  m_light_material.set_uniform(shaders::light::G_NORMAL, 1);
  m_light_material.set_uniform(shaders::light::G_ALBEDO, 2);

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::GAME_ATLAS_SIZE, game_atlas_size);
  m_sector_material.set_uniform(shaders::sector::LEVEL_ATLAS_SIZE, level_atlas_size);
  m_sector_material.set_uniform(shaders::sector::GAME_TEXTURE_ATLAS, 3);
  m_sector_material.set_uniform(shaders::sector::LEVEL_TEXTURE_ATLAS, 4);

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

  // setup directional light ssbo
  glGenBuffers(1, &m_dir_light_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_dir_light_ssbo);
  glBufferData
  (
    GL_SHADER_STORAGE_BUFFER,
    DirectionalLight::MAX_DIRECTIONAL_LIGHTS * sizeof(DirectionalLight::GPUData),
    nullptr,
    GL_DYNAMIC_DRAW
  );

  // setup point light ssbo
  glGenBuffers(1, &m_point_light_ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_point_light_ssbo);
  glBufferData
  (
    GL_SHADER_STORAGE_BUFFER,
    PointLight::MAX_VISIBLE_POINT_LIGHTS * sizeof(PointLight::GPUData),
    nullptr,
    GL_DYNAMIC_DRAW
  );

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//==============================================================================
void Renderer::on_window_resized(u32 w, u32 h)
{
  // clean them up first before recreating
  this->destroy_g_buffers();

  // and recreate with the new size
  this->create_g_buffers(w, h);

  // and recompute projection matrix for this aspect ratio
  this->recompute_projection(w, h, GraphicsSystem::FOV);
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
  do_lighting_pass(camera_data.position);
}

//==============================================================================
const MaterialHandle& Renderer::get_solid_material() const
{
  return m_solid_material;
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
  const GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
  m_g_position = create_g_buffer(GL_RGBA16F, attachments[0], width, height);
  m_g_normal   = create_g_buffer(GL_RGBA16F, attachments[1], width, height);
  m_g_albedo   = create_g_buffer(GL_RGBA,    attachments[2], width, height);
  glDrawBuffers(3, attachments);

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
  m_default_projection = perspective
  (
    fov, aspect, 0.0001f, 100.0f
  );

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
  render_gun(gun);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
  glBindTexture(GL_TEXTURE_2D, m_g_albedo);

  // bind light ssbos
  update_light_ssbos();
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_dir_light_ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_dir_light_ssbo);

  // prepare shader
  m_light_material.use();
  m_light_material.set_uniform(shaders::light::VIEW_POSITION, view_position);
  m_light_material.set_uniform(shaders::light::NUM_DIR_LIGHTS, m_dir_light_ssbo_size);
  m_light_material.set_uniform(shaders::light::NUM_DIR_LIGHTS, m_point_light_ssbo_size);

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
  // TODO: ignore directional lights when indoor

  // get all directional lights
  std::vector<DirectionalLight::GPUData> dir_light_data;
  EntityRegistry& registry = ThingSystem::get().get_entities();
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
    dir_light_data.size() * sizeof(DirectionalLight::GPUData),
    dir_light_data.data()
  );

  /*
   * TODO:
   * - point lights can be even more filtered through compute shader
   *   - it would divide pixels into groups and do sub-frustrum culling for each group
   *   - then during lighting pass we get group of current pixel and just iterate all lights which affects current group
   */

  // update point lights ssbo
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_point_light_ssbo);
  glBufferSubData
  (
    GL_SHADER_STORAGE_BUFFER,
    0,
    m_point_light_data.size() * sizeof(PointLight::GPUData),
    m_point_light_data.data()
  );
  m_point_light_ssbo_size = m_point_light_data.size();

  // clean up
  m_point_light_data.clear();
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//==============================================================================
void Renderer::append_point_light_data(const CameraData& camera) const
{
  const auto& mapping = ThingSystem::get().get_sector_mapping().sectors_to_entities.entities;
  EntityRegistry& registry = ThingSystem::get().get_entities();

  for (const auto& frustum : camera.vis_tree.sectors)
  {
    for (const auto& entity_id : mapping[frustum.sector])
    {
      const PointLight& light = *registry.get_entity(entity_id)->as<PointLight>();
      const vec3 stiched_position = (inverse(camera.view) * vec4(light.get_position(), 1.0f)).xyz;

      m_point_light_data.push_back(light.get_gpu_data(stiched_position));
    }
  }
}

//==============================================================================
void Renderer::render_sectors(const CameraData& camera) const
{
  const auto& sectors_to_render = camera.vis_tree.sectors;
  const std::vector<MeshHandle>& sector_meshes = GraphicsSystem::get().get_sector_meshes();

  const auto& game_atlas = TextureManager::get().get_atlas(ResLifetime::Game);
  const auto& level_atlas = TextureManager::get().get_atlas(ResLifetime::Level);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_texture_data_ssbo);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, game_atlas.handle);
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, level_atlas.handle);

  m_sector_material.use();
  m_sector_material.set_uniform(shaders::sector::VIEW, camera.view);
  m_sector_material.set_uniform(shaders::sector::PORTAL_DEST_TO_SRC, camera.portal_dest_to_src);

  for (const auto& [sector_id, _] : sectors_to_render)
  {
    nc_assert(sector_id < sector_meshes.size());
    const MeshHandle& mesh = sector_meshes[sector_id];

    glBindVertexArray(mesh.get_vao());
    glDrawArrays(mesh.get_draw_mode(), 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
}

//==============================================================================
void Renderer::render_entities(const CameraData& camera) const
{
  constexpr f32 BILLBOARD_TEXTURE_SCALE = 1.0f / 2048.0f;

  // group entities by texture atlas
  const auto& mapping = ThingSystem::get().get_sector_mapping().sectors_to_entities.entities;
  const EntityRegistry& registry = ThingSystem::get().get_entities();

  std::unordered_map<ResLifetime, std::unordered_set<const Entity*>> groups;
  for (const auto& frustum : camera.vis_tree.sectors)
  {
    for (const auto& entity_id : mapping[frustum.sector])
    {
      const Entity* entity = registry.get_entity(entity_id);
      if (const Appearance* appearance = entity->get_appearance())
      {
        const ResLifetime lifetime = appearance->texture.get_lifetime();
        groups[lifetime].insert(entity);
      }
    }
  }

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::VIEW, camera.view);

  const MeshHandle& texturable_quad = MeshManager::get().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());
  glActiveTexture(GL_TEXTURE0);

  const mat3 camera_rotation = transpose(mat3(camera.view));
  // Extracting X and Y components from the forward vector.
  const float yaw = atan2(-camera_rotation[2][0], -camera_rotation[2][2]);
  const mat4 rotation = eulerAngleY(yaw);

  for (const auto& [lifetime, group] : groups)
  {
    const TextureAtlas& atlas = TextureManager::get().get_atlas(lifetime);
    glBindTexture(GL_TEXTURE_2D, atlas.handle);
    m_billboard_material.set_uniform(shaders::billboard::ATLAS_SIZE, atlas.get_size());

    for (const auto* entity : group)
    {
      nc_assert(entity->get_appearance(), "At this point entity must have appearance.");

      const Appearance& appearance = *entity->get_appearance();
      const vec3 position = entity->get_position();

      const TextureHandle& texture = appearance.texture;
      m_billboard_material.set_uniform(shaders::billboard::TEXTURE_POS, texture.get_pos());
      m_billboard_material.set_uniform(shaders::billboard::TEXTURE_SIZE, texture.get_size());

      const vec3 pivot_offset(0.0f, texture.get_height() * BILLBOARD_TEXTURE_SCALE / 2.0f, 0.0f);
      const vec3 scale(
        texture.get_width() * appearance.scale * BILLBOARD_TEXTURE_SCALE,
        texture.get_height() * appearance.scale * BILLBOARD_TEXTURE_SCALE,
        1.0f
      );
      const mat4 transform = translate(mat4(1.0f), position + pivot_offset)
        * rotation
        * nc::scale(mat4(1.0f), scale);
      m_billboard_material.set_uniform(shaders::billboard::TRANSFORM, transform);

      glDrawArrays(texturable_quad.get_draw_mode(), 0, texturable_quad.get_vertex_count());
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
void Renderer::render_gun(const RenderGunProperties& gun) const
{
  if (gun.sprite.empty())
  {
    // Early exit if no gun should be rendered
    return;
  }

  const TextureHandle& texture = TextureManager::get()[gun.sprite];

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  const f32 screen_width = static_cast<f32>(viewport[2]);
  const f32 screen_height = static_cast<f32>(viewport[3]);

  f32 gun_zoom = CVars::gun_zoom;

  vec3 scaling = -vec3{screen_width * gun_zoom, screen_height * gun_zoom, 1.0f};
  f32  trans_x = 0.5f + gun.sway.x * 0.5f;
  f32  trans_y = 0.5f + gun.sway.y * 0.5f;
  vec3 trans   = vec3{trans_x * screen_width, trans_y * screen_height, 0.0f};

  const mat4 projection = ortho(0.0f, screen_width, screen_height, 0.0f, -1.0f, 1.0f);
  const mat4 transform  = translate(mat4{1.0f}, trans) * scale(mat4{1.0f}, scaling);

  const MeshHandle& texturable_quad = MeshManager::get().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::TRANSFORM, transform);
  m_billboard_material.set_uniform(shaders::billboard::VIEW, mat4(1.0f));
  m_billboard_material.set_uniform(shaders::billboard::PROJECTION, projection);
  m_billboard_material.set_uniform(shaders::billboard::ATLAS_SIZE, texture.get_atlas().get_size());
  m_billboard_material.set_uniform(shaders::billboard::TEXTURE_POS, texture.get_pos());
  m_billboard_material.set_uniform(shaders::billboard::TEXTURE_SIZE, texture.get_size());

  glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
  glDrawArrays(texturable_quad.get_draw_mode(), 0, texturable_quad.get_vertex_count());

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  m_billboard_material.set_uniform(shaders::billboard::PROJECTION, m_default_projection);
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