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

		void draw_digits();
		void draw_health();
		void draw_ammo();
		void draw_texts();

		int display_ammo = 0;
		int display_health = 0;

		GLuint VAO;
		GLuint VBO;

		const ShaderProgramHandle digit_shader;
		const ShaderProgramHandle text_shader;
	};
}