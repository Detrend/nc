#include <engine/ui/ui_ammo_display.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>

namespace nc
{
	UiAmmoDisplay::UiAmmoDisplay() :
		shader(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
	{
		init();
	}

	void UiAmmoDisplay::init()
	{
		
	}

	void UiAmmoDisplay::update()
	{
		ammo = get_engine().get_module<ThingSystem>().get_player()->get_current_weapon_ammo();
	}

	void UiAmmoDisplay::draw()
	{

	}
}