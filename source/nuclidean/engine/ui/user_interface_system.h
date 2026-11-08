// Project Nuclidean Source File
#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/ui/ui_menu_manager.h>
#include <engine/ui/ui_screen_effect.h>

#include <engine/ui/ui_hud_display.h>


namespace nc
{
struct ModuleEvent;

class UserInterfaceSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static UserInterfaceSystem& get();
  void on_event(ModuleEvent& event) override;
  bool init();
  ~UserInterfaceSystem() override;

  // Records the whole UI into a draw list and hands it over to the render
  // thread. Called once per frame after the UI got updated.
  void submit_draw_list() const;

  MenuManager* get_menu_manager();
  UiScreenEffect* get_ui_screen_effect();
  UiHudDisplay* get_hud() { return hud_display; }

private:
  MenuManager* menu;
  UiHudDisplay* hud_display;
  UiScreenEffect* screen_effect;
};
}
