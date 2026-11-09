// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <engine/appearance.h>
#include <engine/entity/entity_types.h>
#include <engine/graphics/gl_types.h>
#include <engine/graphics/render_command.h>
#include <engine/graphics/ui_draw_list.h>

#include <memory>
#include <vector>
#include <variant>

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

// Scene-wide element, only collected from persistent proxies.
struct RenderElementSkyBox
{
  GLuint texture              = 0;
  f32    exposure             = 1.0f;
  bool   use_gamma_correction = false;
};

// Scene-wide element, only collected from persistent proxies.
struct RenderElementAmbientLight
{
  f32 strength = 0.0f;
};

struct RenderElementCollector
{
  void push_back(const RenderElementBillboard&    element);
  void push_back(const RenderElementLight&        element);
  void push_back(const RenderElementScreenSprite& element);
  void push_back(const RenderElementSkyBox&       element);
  void push_back(const RenderElementAmbientLight& element);

  void clear();

  std::vector<RenderElementBillboard>    billboards;
  std::vector<RenderElementLight>        lights;
  std::vector<RenderElementScreenSprite> screen_sprites;
  std::vector<RenderElementSkyBox>       sky_boxes;
  std::vector<RenderElementAmbientLight> ambient_lights;
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

  // Camera data can't be stored as a render element because we need to know
  // where is the camera in the scene to do the occlusion culling and then
  // only then start collecting the render elements.
  virtual bool          has_camera_data() const;
  virtual const Camera* get_camera_data() const;

  // Persistent proxies get their render elements collected each frame, no
  // matter if they are visible or not. Has to stay the same for the whole
  // lifetime of the proxy.
  virtual bool is_persistent() const;

private:
  // Necessary to set the ID and the dirty flag
  friend class RenderProxyRegistry;
  friend class GameRenderCommandIface;

  EntityID m_entity_id    = INVALID_ENTITY_ID;
  vec3     m_position     = VEC3_ZERO;
  f32      m_radius       = 0.0f;
  f32      m_height       = 0.0f;
  bool     m_bounds_dirty = false;
};

struct LevelStats
{
  f64 level_time = 0.0; // Seconds since the level started, same as the game time
};

class RenderSettings
{
public:
  enum Setting : u32
  {
    shadows = 1u << 0,
    all = shadows,
  };

  void set_shadows(bool value);
  bool get_shadows() const;

  bool has_changed(Setting setting) const;
  bool has_any_changes()            const;
  void clear_changes();

private:
  bool m_shadows = true;
  u32  m_changes = Setting::all;
};

// Snapshot of one debug gizmo, see GizmoManager. Only produced in builds with
// NC_DEBUG_DRAW, but defined everywhere to keep the command variant simple.
struct RenderGizmo
{
  mat4   transform    = mat4(1.0f);
  color4 color        = colors::WHITE;
  GLuint vao          = 0;
  GLenum draw_mode    = 0;
  u32    vertex_count = 0;
};

// Wipes all proxies, the sectors, the mapping and the level stats on the render side.
// Does not affect the render settings, these are only for the scene, but not for the
// global settings.
struct RenderCommandClearScene
{
};

struct RenderCommandCreateProxy
{
  EntityID                            entity = INVALID_ENTITY_ID;
  std::unique_ptr<IEntityRenderProxy> proxy;
};

struct RenderCommandDestroyProxy
{
  EntityID entity = INVALID_ENTITY_ID;
};

struct RenderCommandModifyProxy
{
  EntityID           entity   = INVALID_ENTITY_ID;
  EntityRenderLambda lambda;
  bool               can_skip = false;
};

struct RenderCommandCreateSectorsProxy
{
  RenderCommandCreateSectorsProxy();
  RenderCommandCreateSectorsProxy(RenderCommandCreateSectorsProxy&&) noexcept;
  RenderCommandCreateSectorsProxy& operator=(RenderCommandCreateSectorsProxy&&) noexcept;
  ~RenderCommandCreateSectorsProxy();

  std::unique_ptr<SectorsRenderProxy> proxy;
};

struct RenderCommandModifySectorsProxy
{
  SectorsRenderLambda lambda;
  bool                can_skip = false;
};

// Modifies the level stats through a lambda.
struct RenderCommandModifyLevelStats
{
  LevelStatsRenderLambda lambda;
};

// Modifies the render settings through a lambda.
struct RenderCommandModifyRenderSettings
{
  RenderSettingsRenderLambda lambda;
};

// Replaces the debug gizmos drawn this frame. Not affected by clearing the scene.
struct RenderCommandSetGizmos
{
  std::vector<RenderGizmo> gizmos;
};

// Probably temporary until we have new UI
struct RenderCommandSetUiDrawList
{
  UiDrawList draw_list;
};

using RenderCommand = std::variant
<
  RenderCommandClearScene,
  RenderCommandCreateProxy,
  RenderCommandDestroyProxy,
  RenderCommandModifyProxy,
  RenderCommandCreateSectorsProxy,
  RenderCommandModifySectorsProxy,
  RenderCommandModifyLevelStats,
  RenderCommandModifyRenderSettings,
  RenderCommandSetGizmos,
  RenderCommandSetUiDrawList
>;

using RenderCommandQueue = std::vector<RenderCommand>;

}
