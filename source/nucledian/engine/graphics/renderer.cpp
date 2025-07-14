// Project Nucledian Source File
#include <engine/graphics/renderer.h>

#include <common.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/core/engine.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/graphics/camera.h>
#include <engine/graphics/gizmo.h>
#include <engine/graphics/graphics_system.h>
#include <engine/map/map_system.h>
#include <engine/player/player.h>
#include <engine/player/thing_system.h>

#include <array>
#include <unordered_set>

namespace nc
{

//==============================================================================
Renderer::Renderer(const GraphicsSystem& graphics_system)
:
  m_solid_material(graphics_system.get_solid_material()),
  m_billboard_material(graphics_system.get_billboard_material())
{}

//==============================================================================
void Renderer::render(const VisibilityTree& visibility_tree) const
{
  const Camera* camera = Camera::get();
  if (!camera)
    return;

  const CameraData camera_data = CameraData
  {
    .position = camera->get_position(),
    .view = camera->get_view(),
    .vis_tree = visibility_tree,
  };

  GizmoManager::instance().draw_gizmos();
  render_sectors(camera_data);
  render_entities(camera_data);
  render_portals(camera_data);
  render_gun();
}

//==============================================================================
void Renderer::render_sectors(const CameraData& camera) const
{
  #pragma region sector colors
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
  #pragma endregion
  const auto& sectors_to_render = camera.vis_tree.sectors;
  const std::vector<MeshHandle>& sector_meshes = GraphicsSystem::get().get_sector_meshes();

  m_solid_material.use();
  m_solid_material.set_uniform(shaders::solid::UNLIT, false);
  m_solid_material.set_uniform(shaders::solid::VIEW, camera.view);
  m_solid_material.set_uniform(shaders::solid::VIEW_POSITION, camera.position);

  for (const auto& [sector_id, _] : sectors_to_render)
  {
    nc_assert(sector_id < sector_meshes.size());
    const MeshHandle& mesh = sector_meshes[sector_id];

    glBindVertexArray(mesh.get_vao());

    const color4& color = SECTOR_COLORS[sector_id % SECTOR_COLORS.size()];
    m_solid_material.set_uniform(shaders::solid::COLOR, color);
    m_solid_material.set_uniform(shaders::solid::TRANSFORM, mat4(1.0f));

    glDrawArrays(mesh.get_draw_mode(), 0, mesh.get_vertex_count());
  }

  glBindVertexArray(0);
}

//==============================================================================
void Renderer::render_entities(const CameraData& camera) const
{
  constexpr f32 BILLBOARD_TEXTURE_SCALE = 1.0f / 2048.0f;

  // group entities by texture
  const auto& mapping = ThingSystem::get().get_sector_mapping().sectors_to_entities.entities;
  const EntityRegistry& registry = ThingSystem::get().get_entities();

  std::unordered_map<u32, std::unordered_set<const Entity*>> groups;
  for (const auto& frustum : camera.vis_tree.sectors)
  {
    for (const auto& entity_id : mapping[frustum.sector])
    {
      const Entity* entity = registry.get_entity(entity_id);
      if (const Appearance* appearance = entity->get_appearance())
      {
        const u32 id = static_cast<u32>(appearance->texture.get_gl_handle());
        groups[id].insert(entity);
      }
    }
  }

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::VIEW, camera.view);

  const MeshHandle& texturable_quad = MeshManager::instance().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());

  const mat3 camera_rotation = transpose(mat3(camera.view));
  // Extracting X and Y components from the forward vector.
  const float yaw = atan2(-camera_rotation[2][0], -camera_rotation[2][2]);
  const mat4 rotation = eulerAngleY(yaw);

  for (const auto& [_, group] : groups)
  {
    const TextureHandle& texture = (*group.begin())->get_appearance()->texture;
    const vec3 pivot_offset(0.0f, texture.get_height() * BILLBOARD_TEXTURE_SCALE / 2.0f, 0.0f);

    glBindTexture(GL_TEXTURE_2D, texture.get_gl_handle());

    for (const auto* entity : group)
    {
      nc_assert(entity->get_appearance(), "At this point entity must have appearance.");

      const Appearance& appearance = *entity->get_appearance();
      const vec3 position = entity->get_position();

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
    };

    render_portal(new_camera, render_data, 0);
  }

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glDisable(GL_STENCIL_TEST);
}

//==============================================================================
void Renderer::render_gun() const
{
  const TextureHandle& texture = TextureManager::instance().get_test_gun_texture();

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  const float screen_width = static_cast<float>(viewport[2]);
  const float screen_height = static_cast<float>(viewport[3]);

  const mat4 projection = ortho(0.0f, screen_width, screen_height, 0.0f, -1.0f, 1.0f);
  const mat4 transform = translate(mat4(1.0f), vec3(screen_width / 2.0f, screen_height / 2.0f, 0.0f))
    * scale(mat4(1.0f), -vec3(screen_width, screen_height, 1.0f));

  const MeshHandle& texturable_quad = MeshManager::instance().get_texturable_quad();
  glBindVertexArray(texturable_quad.get_vao());

  m_billboard_material.use();
  m_billboard_material.set_uniform(shaders::billboard::PROJECTION, projection);
  m_billboard_material.set_uniform(shaders::billboard::VIEW, mat4(1.0f));
  m_billboard_material.set_uniform(shaders::billboard::TRANSFORM, transform);

  glBindTexture(GL_TEXTURE_2D, texture.get_gl_handle());
  glDrawArrays(texturable_quad.get_draw_mode(), 0, texturable_quad.get_vertex_count());

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  m_billboard_material.set_uniform(shaders::billboard::PROJECTION, GraphicsSystem::get().get_default_projection());
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

  const MeshHandle& quad = MeshManager::instance().get_quad();
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

  const MeshHandle& quad = MeshManager::instance().get_quad();
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
    };

    render_portal(new_cam_data, render_data, recursion + 1);
  }
  glStencilFunc(GL_LEQUAL, recursion + 1, 0xFF);

  render_portal_to_depth(camera, portal, false, recursion);
}
#pragma endregion

}