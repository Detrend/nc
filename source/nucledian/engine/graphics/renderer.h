// Project Nucledian Source File
#pragma once

#include <engine/graphics/resources/material.h>

#include <math/vector.h>
#include <math/matrix.h>

#include <memory>
#include <vector>

namespace nc {

struct Portal;
struct VisibilityTree;

class Renderer
{
public:
  Renderer(const GraphicsSystem& graphics_system);

  void render(const VisibilityTree& visibility_tree) const;

private:
  using Portal = Portal;
  struct CameraData;

  const MaterialHandle& m_solid_material;
  const MaterialHandle& m_billboard_material;

  void render_sectors(const CameraData& camera)  const;
  void render_entities(const CameraData& camera) const;
  void render_portals(const CameraData& camera) const;
  void render_gun() const;

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
using RendererPtr = std::unique_ptr<Renderer>;

}
