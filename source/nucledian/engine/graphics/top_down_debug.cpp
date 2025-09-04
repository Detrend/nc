// Project Nucledian Source File
#include <engine/graphics/top_down_debug.h>

#ifdef NC_DEBUG_DRAW

#include <common.h>
#include <metaprogramming.h>

#include <engine/core/engine.h>

#include <engine/map/map_system.h>
#include <engine/graphics/resources/material.h>
#include <engine/graphics/camera.h>

#include <engine/player/thing_system.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <imgui/imgui.h>
#include <glad/glad.h>

namespace nc
{

static MaterialHandle g_top_down_material = MaterialHandle::invalid();
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
  pt = vec2{-1.0f, this->aspect} * (pt - this->pointed_position) * this->zoom;
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
void TopDownDebugRenderer::draw_text(vec2 coords, cstr text, vec3 color)
{
  this->to_screen_space(coords);

  f32 width  = cast<f32>(this->window_width);
  f32 height = cast<f32>(this->window_height);

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
    colors::BLACK,  // directional_light (we don't need to see them)
    colors::YELLOW, // point_light
  };
  static_assert(ARRAY_LENGTH(ENTITY_TYPE_COLORS) == EntityTypes::count);

  ecs.for_each(this->entities_to_render, [&](Entity& entity)
  {
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
  });
}

//==============================================================================
mat4 TopDownDebugRenderer::calc_transform()
{
  return mat4{};
}

//==============================================================================
TopDownDebugRenderer::TopDownDebugRenderer(u32 w, u32 h)
: window_width(w)
, window_height(h)
{
  g_top_down_material = MaterialHandle
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
    pointed_position = vec2{ camera->get_position().x, camera->get_position().z };
    const auto frwd = vec2{ camera->get_forward().x,  camera->get_forward().z };
    player_direction = is_zero(frwd) ? vec2{ 1, 0 } : normalize(frwd);
  }

  auto level_space_to_screen_space = [&](vec2 pos) -> vec2
  {
    return pos;
  };

  if (ImGui::Begin("2D Top Down Debug"))
  {
    ImGui::SliderFloat("Zoom", &zoom, 0.01f, 1.0f);
    ImGui::Separator();
    ImGui::Checkbox("Show visible sectors", &show_visible_sectors);
    ImGui::Checkbox("Show sector frustums", &show_sector_frustums);
    ImGui::Checkbox("Show sector IDs",      &show_sector_ids);
    ImGui::Separator();
    ImGui::Checkbox("Inspect nuclidean portals", &inspect_nucledian_portals);
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
    pointed_position,
    player_direction,
    colors::BLACK,
    colors::ORANGE,
    0.5f
  );

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
