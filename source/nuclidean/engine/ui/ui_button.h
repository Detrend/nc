// Project Nuclidean Source File
#pragma once

#include <engine/game/game_system.h>

#include <functional>
#include <string>

//======================================================================================

namespace nc
{
struct UiDrawList;

class UiButton
{
public:
  UiButton();
  UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func);
  virtual ~UiButton() = default;

  //checks overlap of point and button
  bool is_point_in_rec(vec2 point);

  vec2 get_position();
  vec2 get_scale();

  void set_hover(bool hover);

  // action to be called when a button is pressed
  virtual void on_click();

  // records the button into the UI draw list
  virtual void draw(UiDrawList& out) const;

protected:
  const char* texture_name;
  vec2 position;
  vec2 scale;
  bool isHover = false;

private:
  std::function<void(void)> func;
};

//======================================================================================
class UiLoadGameButton : public UiButton
{
public:
  UiLoadGameButton(const std::string& filename, vec2 position, vec2 scale);

  // action to be called when a button is pressed
  void on_click() override;

  // records the save name into the UI draw list, one quad per character
  void draw(UiDrawList& out) const override;

private:
  std::string save_path;
};

}
