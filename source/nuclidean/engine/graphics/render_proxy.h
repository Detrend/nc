// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <engine/appearance.h>
#include <engine/entity/entity_types.h>
#include <engine/graphics/render_command.h>

#include <memory>
#include <vector>

namespace nc
{

class Camera;

struct RenderElementBillboard
{
  vec3       offset = VEC3_ZERO; // offset from the render proxy position
  Appearance appearance;
};

struct RenderElementLight
{
  vec3   offset = VEC3_ZERO; // offset from the render proxy position
  color3 color;
  f32    intensity;
  f32    radius;
  f32    falloff;
};

struct RenderElementScreenSprite
{
  vec2  offset = VEC2_ZERO;
  f32   scale  = 1.0f;
  Token sprite;
};

struct RenderElementCollector
{
  void push_back(const RenderElementBillboard&    element);
  void push_back(const RenderElementLight&        element);
  void push_back(const RenderElementScreenSprite& element);

  void clear();

  std::vector<RenderElementBillboard>    billboards;
  std::vector<RenderElementLight>        lights;
  std::vector<RenderElementScreenSprite> screen_sprites;
};

class IRenderProxy
{
public:
  virtual ~IRenderProxy() = default;

  virtual void get_render_elements(RenderElementCollector& collector) const = 0;
};

class IEntityRenderProxy : public IRenderProxy
{
public:
  EntityID get_entity_id() const;

  f32  get_radius()   const;
  f32  get_height()   const;
  vec3 get_position() const;

  void set_pos_rad_height(vec3 pos, f32 radius, f32 height);

  virtual bool          has_camera_data() const;
  virtual const Camera* get_camera_data() const;

private: friend class RenderProxyRegistry;
  EntityID m_entity_id    = INVALID_ENTITY_ID;
  vec3     m_position     = VEC3_ZERO;
  f32      m_radius       = 0.0f;
  f32      m_height       = 0.0f;
  bool     m_bounds_dirty = false;
};

enum class RenderCommandType : u8
{
  create_proxy,
  destroy_proxy,
  modify_proxy,
};

struct RenderCommand
{
  EntityID          target_entity = INVALID_ENTITY_ID;
  RenderCommandType type          = RenderCommandType::modify_proxy;

  bool              can_skip      = false;

  std::unique_ptr<IEntityRenderProxy> new_proxy;
  EntityRenderLambda                  lambda;
};

using RenderCommandQueue = std::vector<RenderCommand>;

}
