// Project Nuclidean Source File
#pragma once

#include <engine/graphics/render_proxy.h>
#include <engine/graphics/camera.h>
#include <engine/appearance.h>

#include <token.h>

namespace nc
{

class BillboardRenderProxy : public IEntityRenderProxy
{
public:
  explicit BillboardRenderProxy(const Appearance& appear);

  void get_render_elements(RenderElementCollector& collector) const override;

  Appearance appearance;
};

class PlayerRenderProxy : public BillboardRenderProxy
{
public:
  PlayerRenderProxy(const Appearance& appear, const Camera& cam);

  void get_render_elements(RenderElementCollector& collector) const override;

  bool          has_camera_data() const override;
  const Camera* get_camera_data() const override;

  Camera                    camera;
  RenderElementScreenSprite weapon_sprite;
};

class PointLightRenderProxy : public IEntityRenderProxy
{
public:
  void get_render_elements(RenderElementCollector& collector) const override;

  color3 color                  = colors::WHITE;
  f32    intensity              = 1.0f;
  f32    radius                 = 1.0f;
  f32    falloff                = 1.0f;
  Token  intensity_string;
  f32    intensity_cycle_len    = 0.0f;
  f32    intensity_cycle_offset = 0.0f;

private:
  f32 calc_radius_mod() const;
};

}
