#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/shaders/shaders.h>

namespace nc 
{
	class UiHudDisplay
	{
	public:
		UiHudDisplay();
		~UiHudDisplay();

		void update();

		void draw();
	private:

		void init();

		int display_ammo = 0;
		int display_health = 0;

		GLuint VAO;
		GLuint VBO;

		const ShaderProgramHandle shader;
	};
}