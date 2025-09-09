// Project Nucledian Source File
#pragma once

#include <config.h>

#ifdef NC_DEBUG_DRAW

#include <common.h>
#include <types.h>

#include <math/matrix.h>

#include <engine/entity/entity_types.h>

#include <map>
#include <string>
#include <vector>

namespace nc
{

struct VisibilityTree;

class TopDownDebugRenderer
{
public:
  TopDownDebugRenderer(u32 ww, u32 wh);

  void render(const VisibilityTree& vis);
  void post_render();
  void on_window_resized(u32 new_width, u32 new_height);

  static void push_line(cstr category, vec2 from, vec2 to, vec3 color);

private:
  void draw_line(vec2 from, vec2 to, vec3 color = vec3{1});
  void draw_triangle(vec2 a, vec2 b, vec2 c, vec3 color);
  void draw_text(vec2 coords, cstr text, vec3 color, vec2 scr_offset = vec2{0});

  void draw_player(vec2 coords, vec2 dir, vec3 c1, vec3 c2, f32 scale);
  void draw_custom_objects();
  void draw_entities();
  void draw_sector_grid();

  void to_screen_space(vec2& pt)       const;
  void to_world_space(vec2& screen_pt) const;

  mat3 calc_transform() const;

private:
  struct Line
  {
    vec2 from;
    vec2 to;
    vec3 color;
  };
  inline static std::map<std::string, std::vector<Line>> g_lines_to_draw;
  inline static std::map<std::string, bool> g_enabled_categories;

  bool show_sector_frustums      = true;
  bool show_visible_sectors      = true;
  bool show_sector_grid          = true;
  bool show_sector_grid_list     = false;
  bool show_sector_ids           = false;
  bool inspect_nucledian_portals = false;

  EntityTypeMask entities_to_render = cast<EntityTypeMask>(-1);

  vec2 pointed_position = vec2{0.0f};
  vec2 player_direction = vec2{0.0f};
  f32  zoom             = 0.04f;
  f32  aspect           = 1.0f;
  u32  window_width     = 0;
  u32  window_height    = 0;
};

}

#endif
