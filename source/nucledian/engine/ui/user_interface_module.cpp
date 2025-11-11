#include <engine/ui/user_interface_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/player/thing_system.h>

#include <stb/stb_image.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nc
{
  EngineModuleId nc::UserInterfaceSystem::get_module_id()
  {
    return EngineModule::user_interface_system;
  }

  UserInterfaceSystem& UserInterfaceSystem::get()
  {
    return get_engine().get_module<UserInterfaceSystem>();
  }

  bool UserInterfaceSystem::init()
  {
    menu = new MenuManager();
    ammo_display = new UiHudDisplay();

    return true;
  }

  UserInterfaceSystem::~UserInterfaceSystem()
  {
    delete ammo_display;
    delete menu;
  }

  MenuManager* UserInterfaceSystem::get_menu_manager()
  {
    return menu;
  }

  void UserInterfaceSystem::draw_hud()
  {
    ammo_display->draw();
    menu->draw();
  }

  void UserInterfaceSystem::on_event(ModuleEvent& event)
  {
    switch (event.type)
    {
    case ModuleEventType::post_init:
      break;
    case ModuleEventType::game_update:
      ammo_display->update();
      menu->update();
      break;
    case ModuleEventType::render:
      //draw();
      
      break;
    case ModuleEventType::cleanup:
      break;
    case ModuleEventType::terminate:
      break;
    default:
      break;
    }
  }

}


