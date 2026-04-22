// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/ssbo_buffer.h>
#include <engine/graphics/entities/lights.h>
#include <engine/graphics/resources/texture.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/map/map_types.h>

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
  static constexpr size_t MAX_DIR_LIGHTS = 8;
  static constexpr size_t MAX_VISIBLE_POINT_LIGHTS = 1024;

  // WARNING: Keep value of this constant same as MAX_LIGHTS_PER_TILE in light_culling.comp.
  static constexpr size_t MAX_LIGHTS_PER_TILE = 64;
  static constexpr size_t LIGHT_CULLING_TILE_SIZE_X = 16;
  static constexpr size_t LIGHT_CULLING_TILE_SIZE_Y = 16;

  static constexpr size_t MAX_SECTORS = 1024;
  static constexpr size_t MAX_WALLS = MAX_SECTORS * 8;
  static constexpr size_t MAX_PORTALS = MAX_SECTORS * 4;

  struct CameraData
  {
    const vec3& position;
    const mat4& view;
    const VisibilityTree& vis_tree;
    const mat4& portal_dest_to_src;
    WallID portal_id = INVALID_WALL_ID;
  };

  // The near value has to be very tiny, because otherwise the camera clips into
  // the portals when traversing through them. On the other hand, making it too
  // small will result in losing a lot of depth buffer precision in the long
  // distance, which causes a flicering.
  static constexpr f32 NEAR = 0.001f;
  static constexpr f32 FAR  = 128.0f;

  Renderer(u32 window_w, u32 window_h);

  void on_window_resized(u32 new_width, u32 new_height);
  void render
  (
    const VisibilityTree& visibility_tree,
    const RenderGunProperties& gun
  ) const;

  const ShaderProgramHandle& get_solid_material() const;

  void update_sector_ssbos() const;
  void update_sector_heights(SectorID sector_id) const;

  void set_shadows(bool shadows);

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
    static constexpr f32 DIST_THRESHOLD = 0.25f; // 25cm
    std::pair<bool, size_t> check_redundant(u64 id, vec3 camera_space_pos);
    std::unordered_map<u64, std::vector<vec3>> registry;
  };

  struct SectorGPU
  {
    f32  floor_y;
    f32  ceil_y;
    u32  walls_offset;
    u32  walls_count;
  };

  struct WallGPU
  {
    vec2 start;
    vec2 end;
    u32  packed_normal;
    u32  destination;
    u32  portal_matrix_index;
    u32  _padding;
  };

  mat4  m_default_projection;
  ivec2 m_window_size;
  
  const ShaderProgramHandle m_solid_material;
  const ShaderProgramHandle m_billboard_material;
  const ShaderProgramHandle m_gun_material;
  const ShaderProgramHandle m_light_material;
  const ShaderProgramHandle m_sector_material;
  const ShaderProgramHandle m_light_culling_shader;
  const ShaderProgramHandle m_sky_box_material;

  mutable EntityRedundancyChecker m_light_checker;
  mutable EntityRedundancyChecker m_entity_checker;

  mutable SSBOBuffer<TextureGPU> m_textures_ssbo;
  mutable SSBOBuffer<u32>        m_light_index_ssbo;
  mutable SSBOBuffer<u32>        m_light_tiles_ssbo;
  mutable SSBOBuffer<u32>        m_light_counter_ssbo;

  mutable SSBOBuffer<DirLightGPU>   m_dir_light_ssbo        { MAX_DIR_LIGHTS           };
  mutable SSBOBuffer<PointLightGPU> m_point_light_ssbo      { MAX_VISIBLE_POINT_LIGHTS };
  mutable SSBOBuffer<SectorGPU>     m_sectors_ssbo          { MAX_SECTORS              };
  mutable SSBOBuffer<WallGPU>       m_walls_ssbo            { MAX_WALLS                };
  mutable SSBOBuffer<mat4>          m_portal_matricies_ssbo { MAX_PORTALS              };
  mutable SSBOBuffer<mat4>          m_sector_matricies_ssbo { MAX_SECTORS              };

  mutable std::vector<mat4> m_sector_matricies;;
  mutable std::unordered_map<u64, size_t> m_light_gpu_data_indices;

  GLuint m_g_buffer            = 0;
  GLuint m_g_position          = 0;
  GLuint m_g_stitched_position = 0;
  GLuint m_g_normal            = 0;
  GLuint m_g_stitched_normal   = 0;
  GLuint m_g_albedo            = 0;
  GLuint m_g_sector            = 0;

  bool m_shadows = true;

  void destroy_g_buffers();
  void create_g_buffers(u32 w, u32 h);
  void recompute_projection(u32 screen_w, u32 screen_h, f32 vertical_fov);

  void do_geometry_pass(const CameraData& camera, const RenderGunProperties& gun) const;
  void do_ligh_culling_pass(const CameraData& camera) const;
  void do_lighting_pass(const vec3& view_position) const;

  void update_ssbos() const;

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

