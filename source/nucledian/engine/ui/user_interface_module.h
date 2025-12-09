#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/module_event.h>
#include <engine/ui/ui_texture.h>
#include <engine/ui/ui_button.h>

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

    MenuManager* get_menu_manager();

    void draw_hud();

    UiHudDisplay* get_hud() { return ammo_display; }

  private:
    MenuManager* menu;
    UiHudDisplay* ammo_display;

    std::vector<GuiTexture> ui_elements;

    int display_health = 0;
  };
}