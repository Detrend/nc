// Project Nuclidean Source File
#pragma once
#include <engine/graphics/resources/shader_program.h>

#include <string>
#include <vector>

namespace nc
{

// Presentation made of text slides defined in a JSON file. Each slide has a
// title, a list of points revealed one by one and optionally an image. It is
// rendered with the bitmap UI font over the game, so the demos keep playing in
// the background.
//
// JSON format:
// {
//   "slides": [
//     { "title": "Some title", "points": ["First", "Second"], "image": "texture_name" }
//   ]
// }
// All keys of a slide are optional. The image is a name of a texture from the
// "content/textures" directory. Only ASCII characters can be displayed.
//
// Controls:
// - Space reveals the next point, or goes to the next slide once all points of
//   the current one are visible.
// - Left arrow hides the last visible point, or goes back to the previous
//   slide (with all its points visible) if no point is visible.
class UiTextPresentation
{
public:
  UiTextPresentation();
  ~UiTextPresentation();

  UiTextPresentation(const UiTextPresentation&)            = delete;
  UiTextPresentation& operator=(const UiTextPresentation&) = delete;

  // Loads the slides from a JSON file. Logs the reason and returns false on
  // failure.
  bool load(const std::string& path);

  void update();
  void draw();

private:
  struct Slide
  {
    std::string              title;
    std::vector<std::string> points;
    std::string              image; // texture name, empty if none
  };

  struct TextLine
  {
    std::string text;
    vec2        pos;    // top left corner in pixels
    f32         height; // character height in pixels
    vec4        color;
    s32         point;  // index of the point this line belongs to, -1 for title
  };

  struct SlideLayout
  {
    std::vector<TextLine> lines;
    vec2                  text_min;  // bounds of the text in pixels
    vec2                  text_max;
    vec2                  image_min; // bounds of the image in pixels
    vec2                  image_max;
  };

  void go_forward();
  void go_back();
  void set_slide(u64 slide_idx, u64 visible_points);

  // Positions all lines of the slide, including the not yet revealed points,
  // so the layout does not jump when they appear.
  void layout_slide(const Slide& slide, vec2 window_size, SlideLayout& out) const;

  // Multiplies the color of the given screen rectangle by the brightness.
  void draw_darkened_rect(vec2 min, vec2 max, vec2 window_size, f32 brightness) const;
  void draw_image(const std::string& image, vec2 min, vec2 max, vec2 window_size, f32 alpha) const;
  void draw_text_line(const TextLine& line, vec2 offset, vec4 color, vec2 window_size, f32 time) const;

private:
  std::vector<Slide> slides;
  u64                current_slide  = 0;
  u64                slide_shown_ms = 0; // time when the current slide appeared
  std::vector<u64>   point_shown_ms;     // time when each visible point appeared

  bool prev_space_pressed = false;
  bool prev_left_pressed  = false;

  const ShaderProgramHandle text_material;
  const ShaderProgramHandle image_material;

  GLuint VBO = 0;
  GLuint VAO = 0;
};

}
