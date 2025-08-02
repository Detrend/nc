// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/material.h>

#include <math/vector.h>
#include <math/matrix.h>

#include <memory>
#include <vector>

namespace nc {

struct Portal;
struct VisibilityTree;
struct RenderGunProperties;

class Renderer
{
public:
  Renderer(const GraphicsSystem& graphics_system, u32 window_w, u32 window_h);

  void on_window_resized(u32 new_width, u32 new_height);
  void render
  (
    const VisibilityTree& visibility_tree,
    const RenderGunProperties& gun
  ) const;

  const MaterialHandle& get_solid_material() const;

private:
  using Portal = Portal;
  struct CameraData;

  GLuint m_g_buffer = 0, m_g_position = 0, m_g_normal = 0, m_g_albedo = 0;
  mat4           m_default_projection;
  MaterialHandle m_solid_material;
  MaterialHandle m_billboard_material;
  MaterialHandle m_light_material;

  static GLuint create_g_buffer(GLint internal_format, GLenum attachment, u32 w, u32 h);

  void destroy_g_buffers();
  void create_g_buffers(u32 w, u32 h);
  void recompute_projection(u32 screen_w, u32 screen_h, f32 vertical_fov);

  void do_geometry_pass(const CameraData& camera, const RenderGunProperties& gun) const;
  void do_lighting_pass(const vec3& view_position) const;

  void render_sectors(const CameraData& camera)  const;
  void render_entities(const CameraData& camera) const;
  void render_portals(const CameraData& camera) const;
  void render_gun(const RenderGunProperties& gun) const;

  #pragma region portals rendering
  void render_portal_to_stencil(const CameraData& camera, const Portal& portal, u8 recursion) const;
  void render_portal_to_color(const CameraData& camera, u8 recursion) const;
  void render_portal_to_depth(const CameraData& camera, const Portal& portal, bool depth_write, u8 recursion) const;

  void render_portal(const CameraData& camera, const Portal& portal, u8 recursion) const;
#pragma endregion

  struct CameraData
  {
    const vec3& position;
    const mat4& view;
    const VisibilityTree& vis_tree;
  };
};

}
