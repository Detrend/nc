// Project Nuclidean Source File
#include <engine/ui/user_interface_system.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/render_proxy.h>
#include <engine/graphics/render_proxy_registry.h>

namespace nc
{
EngineModuleId nc::UserInterfaceSystem::get_module_id()
{
  return EngineModule::user_interface_system;
}

//===========================================================================================
UserInterfaceSystem& UserInterfaceSystem::get()
{
  return get_engine().get_module<UserInterfaceSystem>();
}

//===========================================================================================
bool UserInterfaceSystem::init()
{
  menu = new MenuManager();
  hud_display = new UiHudDisplay();
  screen_effect = new UiScreenEffect();

  return true;
}

//===========================================================================================
UserInterfaceSystem::~UserInterfaceSystem()
{
  delete hud_display;
  delete menu;
}
//===========================================================================================
MenuManager* UserInterfaceSystem::get_menu_manager()
{
  return menu;
}

//===========================================================================================
UiScreenEffect* UserInterfaceSystem::get_ui_screen_effect()
{
  return screen_effect;
}

//===========================================================================================
void UserInterfaceSystem::submit_draw_list() const
{
  UiDrawList draw_list;

  if (get_engine().should_ammo_hp_hud_be_visible())
  {
    hud_display->draw(draw_list);
  }

  menu->draw(draw_list);
  screen_effect->draw(draw_list);

  GraphicsSystem::get().get_command_collector().enqueue_command(RenderCommandSetUiDrawList
  {
    .draw_list = std::move(draw_list),
  });
}

//===========================================================================================
void UserInterfaceSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
  case ModuleEventType::post_init:
    menu->post_init();
    break;
  case ModuleEventType::game_update:
    hud_display->update(event.update.dt);
    menu->update();
    screen_effect->update(event.update.dt);
    submit_draw_list();
    break;
  case ModuleEventType::cleanup:
    break;
  case ModuleEventType::terminate:
    menu->on_exit();
    break;
  default:
    break;
  }
}
}


