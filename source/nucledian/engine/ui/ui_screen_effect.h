#pragma once
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/shaders/shaders.h>

namespace nc
{
	class UiScreenEffect
	{
		public:
			UiScreenEffect();
			
			void update(float delta);

			void draw();

			void did_damage(int damage);

		private:
			const f32 MAX_DURATION = 2.0f;
			
			f32 time_since_last_dmg = MAX_DURATION;

			void init();

			GLuint VAO;
			GLuint VBO;

			const ShaderProgramHandle shader;
	};

}