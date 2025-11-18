// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/shader_program.h>

#include <math/vector.h>
#include <math/matrix.h>

#include <unordered_map>
#include <memory>
#include <vector>

namespace nc
{

struct Portal;
struct VisibilityTree;
struct RenderGunProperties;
struct PointLightGPU;

class Renderer
{
public:
  struct CameraData
  {
    const vec3& position;
    const mat4& view;
    const VisibilityTree& vis_tree;
    const mat4& portal_dest_to_src;
  };

  // The near value has to be very tiny, because otherwise the camera clips into
  // the portals when traversing through them. On the other hand, making it too
  // small will result in losing a lot of depth buffer precision in the long
  // distance, which causes a flicering.
  static constexpr f32 NEAR = 0.001f;
  static constexpr f32 FAR = 128.0f;

  Renderer(u32 window_w, u32 window_h);

  void on_window_resized(u32 new_width, u32 new_height);
  void render
  (
    const VisibilityTree& visibility_tree,
    const RenderGunProperties& gun
  ) const;

  const ShaderProgramHandle& get_solid_material() const;

private:
  using Portal = Portal;

  // One entity can be present in multiple sectors, but we want to render it
  // only once. However, it is not sufficient to store only its ID as it can be
  // visible in one sector more times at more places from non euclidean portals.
  // Therefore, we first have to check for the ID and then for its
  // transformation matrix to remove all duplicate entries.
  // If this takes too much processing time then the 4x4 matrix can be replaced
  // by its hash, which could speed it up a little bit (4x4 floats = 64 bytes).
  struct EntityRedundancyChecker
  {
    bool check_redundant(u64 id, mat4 transform);
    std::unordered_map<u64, std::vector<mat4>> registry;
  };

  static constexpr size_t LIGHT_CULLING_TILE_SIZE_X = 8;
  static constexpr size_t LIGHT_CULLING_TILE_SIZE_Y = 8;

  mat4  m_default_projection;
  ivec2 m_window_size;
  
  const ShaderProgramHandle m_solid_material;
  const ShaderProgramHandle m_billboard_material;
  const ShaderProgramHandle m_gun_material;
  const ShaderProgramHandle m_light_material;
  const ShaderProgramHandle m_sector_material;
  const ShaderProgramHandle m_light_culling_shader;
  const ShaderProgramHandle m_sky_box_material;

  mutable u32                        m_dir_light_ssbo_size   = 0;
  mutable u32                        m_point_light_ssbo_size = 0;
  mutable std::vector<PointLightGPU> m_point_light_data;
  mutable EntityRedundancyChecker    m_light_checker;
  mutable EntityRedundancyChecker    m_entity_checker;

  GLuint m_texture_data_ssbo = 0;

  // light pass buffers

  GLuint m_dir_light_ssbo    = 0;
  GLuint m_point_light_ssbo  = 0;
  
  // light culling buffers
 
  GLuint m_light_index_ssbo  = 0;
  GLuint m_tile_data_ssbo    = 0;
  GLuint m_atomic_counter    = 0;

  // g buffers

  GLuint m_g_buffer          = 0;
  GLuint m_g_position        = 0;
  GLuint m_g_normal          = 0;
  GLuint m_g_stitched_normal = 0;
  GLuint m_g_albedo          = 0;


  void destroy_g_buffers();
  void create_g_buffers(u32 w, u32 h);
  void recompute_projection(u32 screen_w, u32 screen_h, f32 vertical_fov);

  void do_geometry_pass(const CameraData& camera, const RenderGunProperties& gun) const;
  void do_ligh_culling_pass(const CameraData& camera) const;
  void do_lighting_pass(const vec3& view_position) const;

  void update_light_ssbos() const;

  void render_sectors(const CameraData& camera)  const;
  void render_entities(const CameraData& camera) const;
  void render_portals(const CameraData& camera) const;
  void render_gun(const CameraData& cam, const RenderGunProperties& gun) const;
  void render_sky_box(const CameraData& camera) const;

  #pragma region portals rendering
  void render_portal_to_stencil(const CameraData& camera, const Portal& portal, u8 recursion) const;
  void render_portal_to_color(const CameraData& camera, u8 recursion) const;
  void render_portal_to_depth(const CameraData& camera, const Portal& portal, bool depth_write, u8 recursion) const;

  void render_portal(const CameraData& camera, const Portal& portal, u8 recursion) const;
  #pragma endregion
};

}

