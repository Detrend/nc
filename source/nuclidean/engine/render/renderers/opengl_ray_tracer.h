// Project Nuclidean Source File
#pragma once

#include <math/vector.h>
#include <engine/map/map_types.h>
#include <engine/render/sdl_types.h>
#include <engine/render/opengl/shader.h>
#include <engine/render/renderers/base_renderer.h>

namespace nc::render::renderers
{

class OpenGLRayTracer : public BaseRenderer
{
public:
  bool init() override;
  void terminate() override;

  void on_window_resolution_changed(uvec2 window_resolution) override;
  void on_virtual_resolution_changed(uvec2 virtual_resolution) override;
  void on_map_rebuilt() override;
  void on_sector_changed(SectorID sector_id) override;
  void on_segment_changed(SegmentID segment_id) override;
  void render() override;

#if NC_DEBUG_DRAW
  f32 get_gpu_time() const override { return m_gpu_time; }
#endif

#if NC_HOT_RELOAD
  void reload_shaders() override;
#endif

private:
  SDL_GLContext m_gl_context = nullptr;

  // Texture the tracing shader renders the frame into.
  GLuint m_output_texture     = 0;
  // Read framebuffer with the output texture attached.
  GLuint m_output_framebuffer = 0;
  // Uniform buffer holding the tracing Context.
  GLuint m_context_buffer     = 0;
  GLuint m_walls_buffer       = 0;
  GLuint m_sectors_buffer     = 0;
  GLuint m_portals_buffer     = 0;

  // Game texture atlas, stored in sRGB.
  GLuint m_atlas_texture        = 0;
  // Placement of each texture within the atlas, indexed by texture id.
  GLuint m_textures_buffer      = 0;
  // Segments of all walls, indexed by segment id.
  GLuint m_wall_segments_buffer = 0;

  // Compute shader tracing the scene into the output texture.
  opengl::Shader m_tracing_shader;
  // Resolution of the output texture.
  uvec2 m_virtual_resolution = uvec2{0, 0};
  // Resolution of the window the output texture is scaled to.
  uvec2 m_window_resolution  = uvec2{0, 0};

#if NC_DEBUG_DRAW
  // Queries measuring the GPU time of a frame, taken in turns so a result is read only once the GPU has it.
  GLuint m_time_queries[3] = {};
  // Number of rendered frames, picks the time query of the frame.
  u64    m_frame_count     = 0;
  // GPU time of rendering a recent frame, in milliseconds.
  f32    m_gpu_time        = 0.0f;
#endif
};

}
