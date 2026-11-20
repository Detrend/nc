// Project Nuclidean Source File
#include <engine/ui/ui_presentation.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/resources/texture.h>

#include <logging.h>

#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <json/json.hpp>

#include <algorithm> // std::clamp, std::max, std::min
#include <cmath>     // std::floor
#include <fstream>   // std::ifstream

namespace nc
{

// Layout, all sizes are relative to the window height
constexpr f32 TITLE_HEIGHT     = 0.075f; // character height of the title
constexpr f32 POINT_HEIGHT     = 0.045f; // character height of the points
constexpr f32 LINE_SPACING     = 1.25f;  // multiple of the character height
constexpr f32 TITLE_GAP        = 0.6f;   // space under the title, multiple of its height
constexpr f32 POINT_GAP        = 0.4f;   // space between points, multiple of their height
constexpr f32 MARGIN           = 0.08f;  // distance of the content from the screen edges
constexpr f32 PANEL_PADDING    = 0.03f;  // padding of the dark panel around the text
constexpr f32 SHADOW_OFFSET    = 0.08f;  // offset of the text shadow, multiple of character height
constexpr f32 TEXT_WIDTH_IMAGE = 0.5f;   // text width relative to the window width if there is an image
constexpr f32 IMAGE_GAP        = 0.04f;  // gap between the text and the image

// The font texture is a grid of 8x16 characters, each 32x48 pixels
constexpr s32 FONT_COLUMNS     = 8;
constexpr s32 FONT_ROWS        = 16;
constexpr f32 CHAR_ASPECT      = 32.0f / 48.0f; // width / height of one character

// Animation
constexpr f32 FADE_IN_MS       = 300.0f; // fade in time of a slide or a point
constexpr f32 SLIDE_IN         = 0.03f;  // horizontal slide in distance of a point, relative to window width
constexpr f32 PANEL_BRIGHTNESS = 0.5f;   // brightness of the game behind the text panel (half transparent)
constexpr f32 WAVE_AMPLITUDE   = 0.0f;   // up-down movement of the letters, relative to window height (0 = disabled, 0.003 is ~3 px at 1080p)
constexpr f32 WAVE_SPEED       = 3.0f;   // radians per second
constexpr f32 WAVE_LENGTH      = 12.0f;  // phase difference across a distance of the window height

constexpr cstr BULLET          = ">";
constexpr vec4 TITLE_COLOR     = vec4{1.0f, 0.75f, 0.3f, 1.0f};
constexpr vec4 BULLET_COLOR    = vec4{1.0f, 0.75f, 0.3f, 1.0f};
constexpr vec4 POINT_COLOR     = vec4{1.0f, 1.0f,  1.0f, 1.0f};
constexpr vec4 SHADOW_COLOR    = vec4{0.0f, 0.0f,  0.0f, 0.8f};

//==============================================================================
UiTextPresentation::UiTextPresentation()
: text_material(ShaderProgramHandle::from_files(shaders::ui_text_colored::VERTEX_FILE, shaders::ui_text_colored::FRAGMENT_FILE))
, image_material(ShaderProgramHandle::from_files(shaders::ui_button::VERTEX_FILE, shaders::ui_button::FRAGMENT_FILE))
{
  // Same quad as the menu uses. The UVs are slightly inset vertically so the
  // neighbouring characters of the font texture do not bleed in.
  vec2 vertices[] =
  {
    vec2(-1.0f,  1.0f), vec2(0.0f, 0.015f),
    vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f - 0.015f),
    vec2( 1.0f,  1.0f), vec2(1.0f, 0.015f),
    vec2( 1.0f, -1.0f), vec2(1.0f, 1.0f - 0.015f),
  };

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

  // position
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  // uvs
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), recast<void*>(2 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//==============================================================================
UiTextPresentation::~UiTextPresentation()
{
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
}

//==============================================================================
bool UiTextPresentation::load(const std::string& path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    nc_crit("Could not open the presentation file \"{}\".", path);
    return false;
  }

  nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
  if (data.is_discarded() || !data.is_object() || !data.contains("slides") || !data.at("slides").is_array())
  {
    nc_crit("Presentation file \"{}\" is not a valid JSON with a \"slides\" array.", path);
    return false;
  }

  const TextureMap& textures = TextureManager::get().get_atlas_bundle(ResLifetime::Game).textures;

  slides.clear();
  u64 replaced_chars = 0;

  for (const nlohmann::json& js : data.at("slides"))
  {
    if (!js.is_object())
    {
      nc_warn("Presentation slide #{} is not a JSON object, skipping it.", slides.size());
      continue;
    }

    Slide slide;

    if (js.contains("title") && js.at("title").is_string())
    {
      slide.title = js.at("title").get<std::string>();
    }

    if (js.contains("points") && js.at("points").is_array())
    {
      for (const nlohmann::json& point : js.at("points"))
      {
        if (point.is_string())
        {
          slide.points.push_back(point.get<std::string>());
        }
        else
        {
          nc_warn("Presentation slide #{} has a point that is not a string, skipping it.", slides.size());
        }
      }
    }

    if (js.contains("image") && js.at("image").is_string())
    {
      std::string image = js.at("image").get<std::string>();
      if (textures.contains(image))
      {
        slide.image = image;
      }
      else
      {
        nc_warn("Presentation slide #{} has an unknown image texture \"{}\", skipping it.", slides.size(), image);
      }
    }

    // The font has only printable ASCII characters. Each other UTF-8 encoded
    // character becomes a single '?'.
    std::vector<std::string*> texts = {&slide.title};
    for (std::string& point : slide.points)
    {
      texts.push_back(&point);
    }

    for (std::string* text : texts)
    {
      std::string ascii;
      for (char character : *text)
      {
        u8 byte = cast<u8>(character);
        if (byte >= 32 && byte <= 126)
        {
          ascii += character;
        }
        else if (character == '\t')
        {
          ascii += ' ';
        }
        else if (byte >= 0xC0)
        {
          // Start of a multi-byte character, its continuation bytes are skipped
          ascii += '?';
          replaced_chars += 1;
        }
      }

      *text = std::move(ascii);
    }

    slides.push_back(std::move(slide));
  }

  if (replaced_chars)
  {
    nc_warn("Presentation contains {} non-ASCII characters, they are displayed as '?'.", replaced_chars);
  }

  if (slides.empty())
  {
    nc_crit("Presentation file \"{}\" contains no slides.", path);
    return false;
  }

  nc_log("Loaded presentation \"{}\" with {} slides.", path, slides.size());

  this->set_slide(0, 0);
  return true;
}

//==============================================================================
void UiTextPresentation::set_slide(u64 slide_idx, u64 visible_points)
{
  u64 now = SDL_GetTicks64();

  current_slide  = slide_idx;
  slide_shown_ms = now;
  point_shown_ms.assign(visible_points, now);
}

//==============================================================================
void UiTextPresentation::go_forward()
{
  const Slide& slide = slides[current_slide];

  if (point_shown_ms.size() < slide.points.size())
  {
    point_shown_ms.push_back(SDL_GetTicks64());
  }
  else if (current_slide + 1 < slides.size())
  {
    this->set_slide(current_slide + 1, 0);
  }
}

//==============================================================================
void UiTextPresentation::go_back()
{
  if (!point_shown_ms.empty())
  {
    point_shown_ms.pop_back();
  }
  else if (current_slide > 0)
  {
    this->set_slide(current_slide - 1, slides[current_slide - 1].points.size());
  }
}

//==============================================================================
void UiTextPresentation::update()
{
  int          num_keys = 0;
  const Uint8* keyboard = SDL_GetKeyboardState(&num_keys);

  bool space = keyboard[SDL_SCANCODE_SPACE];
  bool left  = keyboard[SDL_SCANCODE_LEFT];

  // Edge-triggered: one step per key press
  if (space && !prev_space_pressed)
  {
    this->go_forward();
  }
  else if (left && !prev_left_pressed)
  {
    this->go_back();
  }

  prev_space_pressed = space;
  prev_left_pressed  = left;
}

//==============================================================================
void UiTextPresentation::layout_slide
(
  const Slide& slide, vec2 window_size, SlideLayout& out
) const
{
  f32 margin_x = MARGIN * window_size.y;
  f32 margin_y = MARGIN * window_size.y;

  // The text is on the left, the image takes the rest of the screen
  f32 text_width = slide.image.empty()
    ? window_size.x - 2.0f * margin_x
    : window_size.x * TEXT_WIDTH_IMAGE - margin_x;

  out.lines.clear();
  out.text_min = vec2{margin_x, margin_y};
  out.text_max = out.text_min;

  f32 y = margin_y;

  // Block -1 is the title, then the points follow
  for (s32 block = -1; block < cast<s32>(slide.points.size()); ++block)
  {
    bool               is_title = block < 0;
    const std::string& text     = is_title ? slide.title : slide.points[block];

    if (is_title && text.empty())
    {
      continue;
    }

    f32 height = (is_title ? TITLE_HEIGHT : POINT_HEIGHT) * window_size.y;
    f32 char_w = height * CHAR_ASPECT;

    // Points have a bullet and their wrapped lines are indented behind it
    f32 indent  = is_title ? 0.0f : 2.0f * char_w;
    u64 columns = cast<u64>(std::max(1.0f, std::floor((text_width - indent) / char_w)));

    // Greedy word wrap, too long words are split
    std::vector<std::string> wrapped;
    std::string              current;
    u64                      start = 0;

    while (start <= text.size())
    {
      u64 end = text.find(' ', start);
      if (end == std::string::npos)
      {
        end = text.size();
      }

      std::string word = text.substr(start, end - start);
      start = end + 1;

      while (word.size() > columns)
      {
        if (!current.empty())
        {
          wrapped.push_back(current);
          current.clear();
        }

        wrapped.push_back(word.substr(0, columns));
        word.erase(0, columns);
      }

      if (current.empty())
      {
        current = word;
      }
      else if (current.size() + 1 + word.size() <= columns)
      {
        current += ' ';
        current += word;
      }
      else
      {
        wrapped.push_back(current);
        current = word;
      }
    }

    wrapped.push_back(current);

    if (!is_title)
    {
      out.lines.push_back(TextLine
      {
        .text   = BULLET,
        .pos    = vec2{margin_x, y},
        .height = height,
        .color  = BULLET_COLOR,
        .point  = block,
      });
    }

    for (const std::string& line : wrapped)
    {
      out.lines.push_back(TextLine
      {
        .text   = line,
        .pos    = vec2{margin_x + indent, y},
        .height = height,
        .color  = is_title ? TITLE_COLOR : POINT_COLOR,
        .point  = block,
      });

      out.text_max.x = std::max(out.text_max.x, margin_x + indent + cast<f32>(line.size()) * char_w);
      y += height * LINE_SPACING;
    }

    // The last line spacing is not part of the text
    out.text_max.y = y - height * (LINE_SPACING - 1.0f);
    y += height * (is_title ? TITLE_GAP : POINT_GAP);
  }

  // Fit the image to the area right of the text, keeping its aspect ratio
  out.image_min = vec2{0.0f};
  out.image_max = vec2{0.0f};

  if (!slide.image.empty())
  {
    const TextureHandle& texture = TextureManager::get()[slide.image];

    vec2 area_min  = vec2{window_size.x * TEXT_WIDTH_IMAGE + IMAGE_GAP * window_size.y, margin_y};
    vec2 area_max  = window_size - vec2{margin_x, margin_y};
    vec2 area_size = area_max - area_min;
    vec2 tex_size  = texture.get_size();

    f32  scale      = std::min(area_size.x / tex_size.x, area_size.y / tex_size.y);
    vec2 image_size = tex_size * scale;

    out.image_min = area_min + (area_size - image_size) * 0.5f;
    out.image_max = out.image_min + image_size;
  }
}

//==============================================================================
void UiTextPresentation::draw_darkened_rect
(
  vec2 min, vec2 max, vec2 window_size, f32 brightness
) const
{
  vec2 center = (min + max) * 0.5f;
  vec2 half   = (max - min) * 0.5f;

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::translate(transform, glm::vec3(center.x / window_size.x * 2.0f - 1.0f, 1.0f - center.y / window_size.y * 2.0f, 0.0f));
  transform = glm::scale(transform, glm::vec3(half.x / window_size.x * 2.0f, half.y / window_size.y * 2.0f, 1.0f));

  // The output of the shader is ignored, the blending just multiplies the
  // framebuffer by the brightness.
  image_material.use();
  image_material.set_uniform(shaders::ui_button::TRANSFORM, transform);

  glBlendColor(0.0f, 0.0f, 0.0f, brightness);
  glBlendFunc(GL_ZERO, GL_CONSTANT_ALPHA);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//==============================================================================
void UiTextPresentation::draw_image
(
  const std::string& image, vec2 min, vec2 max, vec2 window_size, f32 alpha
) const
{
  const TextureHandle& texture = TextureManager::get()[image];

  vec2 center = (min + max) * 0.5f;
  vec2 half   = (max - min) * 0.5f;

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::translate(transform, glm::vec3(center.x / window_size.x * 2.0f - 1.0f, 1.0f - center.y / window_size.y * 2.0f, 0.0f));
  transform = glm::scale(transform, glm::vec3(half.x / window_size.x * 2.0f, half.y / window_size.y * 2.0f, 1.0f));

  image_material.use();
  image_material.set_uniform(shaders::ui_button::TRANSFORM, transform);
  image_material.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas_bundle().get_size());
  image_material.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
  image_material.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());
  image_material.set_uniform(shaders::ui_button::HOVER, false);
  image_material.set_uniform(shaders::ui_button::COLOR, vec4{1.0f, 1.0f, 1.0f, alpha});

  glBindTexture(GL_TEXTURE_2D, texture.get_atlas_bundle().diffuse_handle);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

//==============================================================================
void UiTextPresentation::draw_text_line
(
  const TextLine& line, vec2 offset, vec4 color, vec2 window_size, f32 time
) const
{
  // Expects the text material and the font texture to be bound
  f32  char_w    = line.height * CHAR_ASPECT;
  vec2 half      = vec2{char_w / window_size.x, line.height / window_size.y};
  f32  amplitude = WAVE_AMPLITUDE * window_size.y;

  text_material.set_uniform(shaders::ui_text_colored::COLOR, color);

  for (u64 i = 0; i < line.text.size(); ++i)
  {
    if (line.text[i] == ' ')
    {
      continue;
    }

    vec2 center = line.pos + offset + vec2{(cast<f32>(i) + 0.5f) * char_w, 0.5f * line.height};

    // Each letter moves slightly up and down, the phase depends on its position
    // so a gentle wave travels through the text. Computed from the position
    // without the offset, so the shadow moves together with its letter.
    vec2 base_pos = (line.pos + vec2{(cast<f32>(i) + 0.5f) * char_w, 0.0f}) / window_size.y;
    center.y += amplitude * std::sin(time * WAVE_SPEED - (base_pos.x + base_pos.y) * WAVE_LENGTH);

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(center.x / window_size.x * 2.0f - 1.0f, 1.0f - center.y / window_size.y * 2.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(half.x, half.y, 1.0f));

    text_material.set_uniform(shaders::ui_text_colored::TRANSFORM, transform);
    text_material.set_uniform(shaders::ui_text_colored::CHARACTER, cast<s32>(line.text[i]));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}

//==============================================================================
void UiTextPresentation::draw()
{
  if (slides.empty())
  {
    return;
  }

  const Slide& slide       = slides[current_slide];
  vec2         window_size = get_engine().get_module<GraphicsSystem>().get_window_size();
  u64          now         = SDL_GetTicks64();
  f32          time        = cast<f32>(now % 3'600'000) / 1000.0f; // seconds, wrapped to keep float precision

  SlideLayout layout;
  this->layout_slide(slide, window_size, layout);

  // Fade in of the whole slide, eased out
  f32 slide_t = std::clamp(cast<f32>(now - slide_shown_ms) / FADE_IN_MS, 0.0f, 1.0f);
  slide_t     = slide_t * (2.0f - slide_t);

  glBindVertexArray(VAO);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);

  // Dark panel behind the text so it is readable over the game
  if (!layout.lines.empty())
  {
    vec2 padding = vec2{PANEL_PADDING * window_size.y};
    f32  dim     = 1.0f - (1.0f - PANEL_BRIGHTNESS) * slide_t;
    this->draw_darkened_rect(layout.text_min - padding, layout.text_max + padding, window_size, dim);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (!slide.image.empty())
  {
    this->draw_image(slide.image, layout.image_min, layout.image_max, window_size, slide_t);
  }

  // Text
  const TextureHandle& font = TextureManager::get()["ui_font"];

  text_material.use();
  text_material.set_uniform(shaders::ui_text_colored::ATLAS_SIZE, font.get_atlas_bundle().get_size());
  text_material.set_uniform(shaders::ui_text_colored::TEXTURE_POS, font.get_pos());
  text_material.set_uniform(shaders::ui_text_colored::TEXTURE_SIZE, font.get_size());
  text_material.set_uniform(shaders::ui_text_colored::WIDTH, FONT_COLUMNS);
  text_material.set_uniform(shaders::ui_text_colored::HEIGHT, FONT_ROWS);
  glBindTexture(GL_TEXTURE_2D, font.get_atlas_bundle().diffuse_handle);

  for (const TextLine& line : layout.lines)
  {
    f32  alpha  = slide_t;
    vec2 offset = vec2{0.0f};

    if (line.point >= 0)
    {
      if (cast<u64>(line.point) >= point_shown_ms.size())
      {
        continue; // Not revealed yet
      }

      // Each point fades and slides in when revealed
      f32 point_t = std::clamp(cast<f32>(now - point_shown_ms[line.point]) / FADE_IN_MS, 0.0f, 1.0f);
      point_t     = point_t * (2.0f - point_t);

      alpha  = std::min(alpha, point_t);
      offset = vec2{(1.0f - point_t) * SLIDE_IN * window_size.x, 0.0f};
    }

    vec2 shadow_offset = offset + vec2{SHADOW_OFFSET * line.height};
    vec4 shadow_color  = vec4{SHADOW_COLOR.r, SHADOW_COLOR.g, SHADOW_COLOR.b, SHADOW_COLOR.a * alpha};
    vec4 color         = vec4{line.color.r, line.color.g, line.color.b, line.color.a * alpha};

    this->draw_text_line(line, shadow_offset, shadow_color, window_size, time);
    this->draw_text_line(line, offset, color, window_size, time);
  }

  // Restore the state
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindVertexArray(0);
}

}
