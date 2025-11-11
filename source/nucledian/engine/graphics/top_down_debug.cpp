// Project Nucledian Source File
#include <engine/graphics/top_down_debug.h>

#ifdef NC_DEBUG_DRAW

#include <common.h>
#include <metaprogramming.h>

#include <engine/core/engine.h>

#include <engine/map/map_system.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/camera.h>

#include <engine/player/thing_system.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/enemies/enemy.h>

#include <imgui/imgui.h>
#include <glad/glad.h>

#include <string>
#include <format>
#include <cmath>    // std::abs
#include <iterator> // std::next
#include <numeric>  // std::accumulate

namespace nc
{

static ShaderProgramHandle g_top_down_material = ShaderProgramHandle::invalid();
static GLuint         g_default_vao       = 0;

//==============================================================================
constexpr cstr TOP_DOWN_FRAGMENT_SOURCE = R"ABC(
#version 430 core
out vec4 FragColor;

layout(location = 0) uniform vec3 u_color;

void main()
{
  FragColor = vec4(u_color, 1.0f);
} 
)ABC";

//==============================================================================
constexpr cstr TOP_DOWN_VERTEX_SOURCE = R"ABC(
#version 430 core
layout (location = 0) in vec3 aPos;

void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)ABC";

//==============================================================================
void TopDownDebugRenderer::draw_line(vec2 from, vec2 to, vec3 color)
{
  this->to_screen_space(from);
  this->to_screen_space(to);

  g_top_down_material.use();

  const auto  screen_bbox = aabb2{ vec2{-1}, vec2{1} };
  const aabb2 bbox = aabb2{ from, to };

  if (!intersect::aabb_aabb_2d(bbox, screen_bbox))
  {
    return;
  }

  glBindVertexArray(g_default_vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[2] = { vec3(from, -0.5f), vec3(to, -0.5f) };
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 2, data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glUniform3f(0, color.x, color.y, color.z);

  glDrawArrays(GL_LINES, 0, 2);

  glDeleteBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//==============================================================================
void TopDownDebugRenderer::to_screen_space(vec2& pt) const
{
  pt = (this->calc_transform() * vec3{pt.x, pt.y, 1.0f}).xy();
}

//==============================================================================
void TopDownDebugRenderer::to_world_space(vec2& pt) const
{
  pt = (inverse(this->calc_transform()) * vec3{pt.x, pt.y, 1.0f}).xy();
}

//==============================================================================
void TopDownDebugRenderer::draw_triangle(vec2 a, vec2 b, vec2 c, vec3 color)
{
  this->to_screen_space(a);
  this->to_screen_space(b);
  this->to_screen_space(c);

  g_top_down_material.use();

  const auto  screen_bbox = aabb2{ vec2{-1}, vec2{1} };
  const aabb2 bbox = aabb2{ a, b, c };

  if (!intersect::aabb_aabb_2d(bbox, screen_bbox))
  {
    return;
  }

  glBindVertexArray(g_default_vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[3] =
  {
    vec3(a, -0.5f),
    vec3(b, -0.5f),
    vec3(c, -0.5f),
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glUniform3f(0, color.x, color.y, color.z);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDeleteBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//==============================================================================
void TopDownDebugRenderer::draw_text(vec2 coords, cstr text, vec3 color, vec2 off)
{
  this->to_screen_space(coords);

  if (std::abs(coords.x) > 1.0f || std::abs(coords.y) > 1.0f)
  {
    return;
  }

  f32 width  = cast<f32>(this->window_width);
  f32 height = cast<f32>(this->window_height);

  coords += off / vec2{width, height};

  const auto col = ImColor{color.x, color.y, color.z, 1.0f};

  coords = (coords * vec2{ 1.0f, -1.0f } + vec2{ 1 }) * vec2{ 0.5f } *vec2{ width, height };
  ImGui::GetForegroundDrawList()->AddText(ImVec2{ coords.x, coords.y }, col, text);
};

//==============================================================================
void TopDownDebugRenderer::draw_player
(
  vec2 coords, vec2 dir, vec3 color1, vec3 color2, f32 scale
)
{
  vec2 front = coords + dir * scale;
  vec2 back  = coords - dir * 0.5f * scale;
  vec2 left  = coords + flipped(dir) * scale * 0.5f - dir * scale;
  vec2 right = coords - flipped(dir) * scale * 0.5f - dir * scale;

  this->draw_triangle(front, back, left, color1);
  this->draw_triangle(front, back, right, color1);

  this->draw_line(front, left, color2);
  this->draw_line(left, back, color2);
  this->draw_line(back, right, color2);
  this->draw_line(right, front, color2);

  // axes
  this->draw_line(coords, coords + VEC2_X, colors::RED);
  this->draw_line(coords, coords + VEC2_Y, colors::GREEN);
}

//==============================================================================
void TopDownDebugRenderer::draw_custom_objects()
{
  for (const auto&[category, set] : g_lines_to_draw)
  {
    if (g_enabled_categories[category])
    {
      for (Line line : set)
      {
        this->draw_line(line.from, line.to, line.color);
      }
    }
  }
}

//==============================================================================
void TopDownDebugRenderer::draw_entities()
{
  auto& game = ThingSystem::get();
  auto& ecs  = game.get_entities();

  constexpr vec4 ENTITY_TYPE_COLORS[]
  {
    colors::WHITE,  // player
    colors::ORANGE, // enemy
    colors::GREEN,  // pickup
    colors::TEAL,   // projectile
    colors::BLACK,  // ambient_light (we don't need to see them)
    colors::BLACK,  // directional_light (we don't need to see them)
    colors::YELLOW, // point_light
    colors::PINK,   // prop
  };
  static_assert(ARRAY_LENGTH(ENTITY_TYPE_COLORS) == EntityTypes::count);

  ecs.for_each(this->entities_to_render, [&](Entity& entity)
  {
    EntityID   id     = entity.get_id();
    EntityType type   = entity.get_type();
    f32        radius = entity.get_radius();
    vec3       color  = ENTITY_TYPE_COLORS[type].xyz();
    vec2       pos    = entity.get_position().xz();

    vec2 lt = vec2{-1.0f,  1.0f} * radius;
    vec2 rt = vec2{ 1.0f,  1.0f} * radius;
    vec2 lb = vec2{-1.0f, -1.0f} * radius;
    vec2 rb = vec2{ 1.0f, -1.0f} * radius;

    this->draw_line(pos + lt, pos + rt, color);
    this->draw_line(pos + rt, pos + rb, color);
    this->draw_line(pos + rb, pos + lb, color);
    this->draw_line(pos + lb, pos + lt, color);

    // Draw the direction for enemies
    if (Enemy* enemy = entity.as<Enemy>())
    {
      this->draw_line(pos, pos + enemy->get_facing().xz(), colors::WHITE);
    }

    if (this->show_entity_ids)
    {
      auto entity_str = std::format("{}:{}", ENTITY_TYPE_NAMES[type], id.idx);
      this->draw_text
      (
        pos,
        entity_str.c_str(),
        color
      );
    }
  });
}

//==============================================================================
void TopDownDebugRenderer::draw_sector_grid()
{
  auto& map = ThingSystem::get().get_map();

  if (map.sector_grid.m_cells.empty())
  {
    return;
  }

  vec2 from = map.sector_grid.m_min;
  vec2 to   = map.sector_grid.m_max;
  s64  w    = cast<s64>(map.sector_grid.m_cells.size());
  s64  h    = cast<s64>(map.sector_grid.m_cells[0].size());
  f32  cell_w = (to.x - from.x) / w;
  f32  cell_h = (to.y - from.y) / h;

  vec2 screen_from = vec2{-1, -1};
  vec2 screen_to   = vec2{ 1,  1};
  this->to_world_space(screen_from);
  this->to_world_space(screen_to);

  vec2 ws_screen_from = min(screen_from, screen_to);
  vec2 ws_screen_to   = max(screen_from, screen_to);

  vec2 from_beg = max(ws_screen_from - from, VEC2_ZERO);
  vec2 to_end   = max(to - ws_screen_to,     VEC2_ZERO);

  s64 beg_off_x = cast<s64>(from_beg.x / cell_w);
  s64 beg_off_y = cast<s64>(from_beg.y / cell_h);
  s64 end_off_x = cast<s64>(to_end.x   / cell_w);
  s64 end_off_y = cast<s64>(to_end.y   / cell_h);

  // Vertical lines
  for (s64 i = beg_off_x; i <= w - end_off_x; ++i)
  {
    f32 coeff = cast<f32>(i) / w;
    f32 x     = coeff * to.x + (1.0f - coeff) * from.x;
    this->draw_line(vec2{x, from.y}, vec2{x, to.y}, colors::WHITE);
  }

  // Horizontal lines
  for (s64 i = beg_off_y; i <= h - end_off_y; ++i)
  {
    f32 coeff = cast<f32>(i) / h;
    f32 y     = coeff * to.y + (1.0f - coeff) * from.y;
    this->draw_line(vec2{from.x, y}, vec2{to.x, y}, colors::WHITE);
  }

  // Number of sectors
  for (s64 x = beg_off_x; x <= w - end_off_x; ++x)
  {
    for (s64 y = beg_off_y; y <= h - end_off_y; ++y)
    {
      f32 cx  = cast<f32>(x) / w;
      f32 xp  = cx * to.x + (1.0f - cx) * from.x + cell_w * 0.5f;
      f32 cy  = cast<f32>(y) / h;
      f32 yp  = cy * to.y + (1.0f - cy) * from.y + cell_h * 0.5f;

      auto& cells = map.sector_grid.m_cells[x][y];
      u64 cnt = cells.size();

      std::string coord = std::format("({},{})", x, y);
      std::string txt   = std::to_string(cnt);
      this->draw_text(vec2{xp, yp}, coord.c_str(), colors::GRAY);

      if (cnt)
      {
        this->draw_text(vec2{xp, yp}, txt.c_str(), colors::RED, vec2{0, -25});

        if (this->show_sector_grid_list)
        {
          std::string list_of_sectors = std::accumulate
          (
            std::next(cells.begin()),
            cells.end(),
            std::to_string(cells.front().data),
            [](std::string a, auto b)
            {
              return std::move(a) + "," + std::to_string(b.data);
            }
          );

          this->draw_text
          (
            vec2{xp, yp}, list_of_sectors.c_str(), colors::BLUE, vec2{-150, -50}
          );
        }
      }
    }
  }
}

//==============================================================================
mat3 TopDownDebugRenderer::calc_transform() const
{
  mat3 translate = mat3
  {
    vec3{1.0f, 0.0f, 0.0f},
    vec3{0.0f, 1.0f, 0.0f},
    vec3{-this->player_position, 1.0f}
  };

  vec3 c0 = vec3{1.0f, 0.0f, 0.0f} * -1.0f        * this->zoom;
  vec3 c1 = vec3{0.0f, 1.0f, 0.0f} * this->aspect * this->zoom;
  mat3 scaling = mat3{c0, c1, vec3{0, 0, 1}};

  return scaling * translate;
}

//==============================================================================
TopDownDebugRenderer::TopDownDebugRenderer(u32 w, u32 h)
: window_width(w)
, window_height(h)
{
  g_top_down_material = ShaderProgramHandle
  (
    TOP_DOWN_VERTEX_SOURCE,
    TOP_DOWN_FRAGMENT_SOURCE
  );
  glGenVertexArrays(1, &g_default_vao);

  this->aspect = cast<f32>(w) / h;
}

//==============================================================================
void TopDownDebugRenderer::render(const VisibilityTree& visible_sectors)
{
  glClear(GL_STENCIL_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  GLfloat original_line_width = 0;
  glGetFloatv(GL_LINE_WIDTH, &original_line_width);

  glLineWidth(1.0f);

  if (auto* camera = Camera::get())
  {
    player_position = vec2{ camera->get_position().x, camera->get_position().z };
    const auto frwd = vec2{ camera->get_forward().x,  camera->get_forward().z };
    player_direction = is_zero(frwd) ? vec2{ 1, 0 } : normalize(frwd);
  }

  if (ImGui::Begin("2D Top Down Debug"))
  {
    ImGui::SliderFloat("Zoom", &zoom, 0.01f, 1.0f);
    ImGui::Separator();
    ImGui::Checkbox("Show visible sectors", &this->show_visible_sectors);
    ImGui::Checkbox("Show sector frustums", &this->show_sector_frustums);
    ImGui::Checkbox("Show sector IDs",      &this->show_sector_ids);
    ImGui::Checkbox("Show entity IDs",      &this->show_entity_ids);
    ImGui::Checkbox("Show sector grid",     &this->show_sector_grid);
    if (this->show_sector_grid)
    {
      ImGui::Checkbox("Show list of sectors", &this->show_sector_grid_list);
    }

    ImGui::Separator();
    ImGui::Checkbox("Inspect nuclidean portals", &inspect_nucledian_portals);

    ImGui::Separator();
    ImGui::Checkbox("Show path debug", &this->show_path_debug);
    if (this->show_path_debug)
    {
      ImGui::Checkbox("Path Smoothing", &this->do_path_smoothing);

      if (ImGui::Button("Set Start"))
      {
        this->path_debug_start = player_position;
      }

      if (ImGui::Button("Set End"))
      {
        this->path_debug_end = player_position;
      }

      if (ImGui::Button("Set End To Player"))
      {
        this->path_debug_end = VEC2_ZERO;
      }
    }

    ImGui::Separator();
    ImGui::Text("Categories");

    for (auto&[name, value] : g_enabled_categories)
    {
      bool val = value;
      if (ImGui::Checkbox(name.c_str(), &val))
      {
        value = val;
      }
    }
  }
  ImGui::End();

  auto& map = get_engine().get_map();
  auto  lvl = ThingSystem::get().get_level();

  // Render the floors of the visible_sectors with black or gray if visible_sectors
  for (SectorID i = 0; i < map.sectors.size(); ++i)
  {
    const auto& sector = map.sectors[i];
    const auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    nc_assert(wall_count >= 3);

    const bool is_visible = visible_sectors.is_visible(i);
    const vec3 color = (is_visible && show_visible_sectors) ? vec3{ 0.25f } : vec3{ colors::BLACK };

    const auto& first_wall = map.walls[repr.first_wall];
    vec2 avg_position = first_wall.pos;

    for (WallID index = 1; index < wall_count; ++index)
    {
      WallID next_index = (index + 1) % wall_count;
      WallID index_in_arr = repr.first_wall + index;
      WallID next_in_arr = repr.first_wall + next_index;
      nc_assert(index_in_arr < map.walls.size());
      nc_assert(next_in_arr < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      avg_position = avg_position + wall1.pos;

      draw_triangle(
        first_wall.pos,
        wall1.pos,
        wall2.pos,
        color
      );
    }

    avg_position = avg_position / static_cast<f32>(wall_count);
    if (show_sector_ids)
    {
      const auto sector_color = is_visible ? colors::WHITE : colors::PINK;
      const auto sector_str = std::to_string(i);
      draw_text
      (
        avg_position,
        sector_str.c_str(),
        sector_color
      );
    }
  }

  // Render frustums for all visible_sectors visible_sectors
  if (show_sector_frustums)
  {
    glEnable(GL_STENCIL_TEST);

    for (const auto& [sector_id, frustums] : visible_sectors.sectors)
    {
      glStencilMask(0xFF);
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glColorMask(false, false, false, false); // do not write into the color buffer

      glClearStencil(0);
      glClear(GL_STENCIL_BUFFER_BIT);

      // render the sector shape into stencil buffer
      {
        const auto& sector = map.sectors[sector_id];
        const auto& repr = sector.int_data;
        const s32 wall_count = repr.last_wall - repr.first_wall;
        nc_assert(wall_count >= 3);

        const auto& first_wall = map.walls[repr.first_wall];

        for (WallID index = 1; index < wall_count; ++index)
        {
          WallID next_index = (index + 1) % wall_count;
          WallID index_in_arr = repr.first_wall + index;
          WallID next_in_arr = repr.first_wall + next_index;
          nc_assert(index_in_arr < map.walls.size());
          nc_assert(next_in_arr < map.walls.size());

          const auto& wall1 = map.walls[index_in_arr];
          const auto& wall2 = map.walls[next_in_arr];
          draw_triangle(
            first_wall.pos,
            wall1.pos,
            wall2.pos,
            vec3{ 1 }
          );
        }
      }

      // render the frustum only on parts where the stencil buffer is enabled
      {
        glColorMask(true, true, true, true);    // turn on the color back again
        glStencilFunc(GL_EQUAL, 1, 0xFF);       // fail the test if the value is not 1
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // keep the value in stencil buffer even if we fail the test

        // render the frustum, but with the stencil test, so only the pixels inside the sector pass
        for (const auto& frustum : frustums.frustum_slots)
        {
          if (frustum == INVALID_FRUSTUM)
          {
            continue;
          }

          vec2 le, re;
          frustum.get_frustum_edges(le, re);

          const auto color = vec3{ 0.5f, 0.5f, 0.5f };
          draw_triangle(
            frustum.center,
            frustum.center + le * 3000.0f,
            frustum.center + re * 3000.0f,
            color
          );
        }
      }
    }

    // cleanup the stencil buffer after ourselves
    {
      glClear(GL_STENCIL_BUFFER_BIT);
      glDisable(GL_STENCIL_TEST);
    }
  }

  // then render the walls with white
  for (auto&& sector : map.sectors)
  {
    constexpr color4 PORTAL_TYPES_COLORS[3] = {colors::WHITE, colors::RED, colors::GREEN};

    auto& repr = sector.int_data;
    const s32 wall_count = repr.last_wall - repr.first_wall;
    nc_assert(wall_count >= 0);

    for (WallRelID index = 0; index < wall_count; ++index)
    {
      WallRelID next_index   = (index + 1) % wall_count;
      WallID    index_in_arr = repr.first_wall + index;
      WallID    next_in_arr  = repr.first_wall + next_index;
      nc_assert(index_in_arr < map.walls.size());
      nc_assert(next_in_arr < map.walls.size());

      const auto& wall1 = map.walls[index_in_arr];
      const auto& wall2 = map.walls[next_in_arr];

      const auto portal_type = wall1.get_portal_type();
      const auto color = PORTAL_TYPES_COLORS[portal_type];

      draw_line(wall1.pos, wall2.pos, color);
    }
  }

  // and render the player
  this->draw_player
  (
    player_position,
    player_direction,
    colors::BLACK,
    colors::ORANGE,
    0.5f
  );

  if (this->show_sector_grid)
  {
    this->draw_sector_grid();
  }

  if (this->show_path_debug)
  {
    vec3 from = vec3{this->path_debug_start.x, 0.0f, this->path_debug_start.y};
    vec3 to   = vec3{this->path_debug_end.x,   0.0f, this->path_debug_end.y  };

    if (this->path_debug_end == VEC2_ZERO)
    {
      to = vec3{player_position.x, 0.0f, player_position.y};
    }

    std::vector<vec3> path = map.get_path(from, to, 0.25f, 1.0f);
    path.insert(path.begin(), from);
    if (this->do_path_smoothing)
    {
      lvl.smooth_out_path(path, 0.25f, 1.0f);
    }

    for (u64 i = 1; i < path.size(); ++i)
    {
      vec3 curr = path[i];
      vec3 prev = path[i-1];
      this->draw_line(curr.xz(), prev.xz(), colors::LIME);
    }
  }

  // render entities
  this->draw_entities();

  // render the custom objects
  this->draw_custom_objects();

  glLineWidth(original_line_width);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

//==============================================================================
void TopDownDebugRenderer::post_render()
{
  g_lines_to_draw.clear();
}

//==============================================================================
void TopDownDebugRenderer::on_window_resized(u32 w, u32 h)
{
  this->aspect = cast<f32>(w) / h;
  this->window_width  = w;
  this->window_height = h;
}

//==============================================================================
/*static*/ void TopDownDebugRenderer::push_line
(
  cstr category, vec2 from, vec2 to, vec3 c
)
{
  if (!g_enabled_categories.contains(category))
  {
    g_enabled_categories[category] = false;
  }

  g_lines_to_draw[category].push_back(Line{from, to, c});
}

}

#endif
