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

			void did_pickup();

		private:
			const f32 MAX_DMG_FLASH_DURATION = 2.0f;
			const f32 MAX_PICKUP_FLASH_DURATION = 0.25f;

			f32 time_since_last_dmg = MAX_DMG_FLASH_DURATION;
			f32 time_since_last_pickup = MAX_PICKUP_FLASH_DURATION;

			void init();

			GLuint VAO;
			GLuint VBO;

			const ShaderProgramHandle shader;
	};

}