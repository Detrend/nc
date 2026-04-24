// Project Nuclidean Source File
#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/ui/ui_menu_manager.h>
#include <engine/ui/ui_screen_effect.h>

#include <vector>

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
    ~UserInterfaceSystem();

    void draw();

    MenuManager* get_menu_manager();
    UiScreenEffect* get_ui_screen_effect();
    UiHudDisplay* get_hud() { return hud_display; }

  private:
    MenuManager* menu;
    UiHudDisplay* hud_display;
    UiScreenEffect* screen_effect;
  };
}