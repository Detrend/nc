#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/shaders/shaders.h>

namespace nc 
{
	class UiAmmoDisplay
	{
	public:
		UiAmmoDisplay();

		void update();

		void draw();
	private:

		void init();

		int ammo = 0;

		const ShaderProgramHandle shader;
	};
}