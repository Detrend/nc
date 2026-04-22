#pragma once

#include <engine/ui/ui_menu_page.h>
#include <engine/ui/user_interface_system.h>

#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <engine/game/game_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/game/game_helpers.h>
#include <engine/core/engine.h>
#include <engine/input/input_system.h>
#include <engine/sound/sound_system.h>
#include <engine/core/module_event.h>
#include <engine/graphics/shaders/shaders.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <format>
#include <json/json.hpp>

namespace nc
{

	NextLevelPage::NextLevelPage()
	{
		next_level_button = new UiButton("ui_next_level", vec2(0.0f, -0.8f), vec2(0.3f, 0.066f), std::bind(&NextLevelPage::next_level_func, this));
		menu_button = new UiButton("ui_menu", vec2(0.0f, -0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::menu_func, this));
		kills_text = new UiButton("ui_kills", vec2(-0.3f, 0.3f), vec2(0.30f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		level_text = new UiButton("ui_level", vec2(0.0f, 0.8f), vec2(0.15f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		demo_text = new UiButton("ui_demo", vec2(0.0f, 0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		completed_text = new UiButton("ui_completed", vec2(0.0f, 0.6f), vec2(0.253f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
	}

	//=============================================================================================

	NextLevelPage::~NextLevelPage()
	{
		delete next_level_button;
		delete kills_text;
		delete demo_text;
		delete level_text;
		delete completed_text;
		delete menu_button;
	}

	//=============================================================================================

	void NextLevelPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
	{
		// determines wheter next_level or back_to_menu button should be updated
		UiButton* hover_over_button = nullptr;

		if (get_engine().get_module<GameSystem>().get_level_name() != Levels::LEVEL_3)
		{
			next_level_button->set_hover(false);

			if (next_level_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = next_level_button;
			}
		}
		else
		{
			menu_button->set_hover(false);

			if (menu_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = menu_button;
			}
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}
	}

	//=============================================================================================

	void NextLevelPage::draw(ShaderProgramHandle button_material, ShaderProgramHandle digit_material, GLuint VAO)
	{
		// shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// determines wheter next_level or back_to_menu button should be rendered
		kills_text->draw(button_material);
		if (get_engine().get_module<GameSystem>().get_level_name() == Levels::LEVEL_3)
		{
			demo_text->draw(button_material);
			menu_button->draw(button_material);
		}
		else
		{
			level_text->draw(button_material);
			next_level_button->draw(button_material);
		}
		completed_text->draw(button_material);

		// shader unbinding
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		draw_stats(digit_material, VAO);
	}

	//=============================================================================================

	void NextLevelPage::set_kill_stats(u32 enemies, u32 kills)
	{
		enemy_count = enemies;
		kill_count = kills;
	}

	//=============================================================================================

	void NextLevelPage::draw_stats(ShaderProgramHandle digit_material, GLuint VAO)
	{
		// shader init
		digit_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		draw_kill_count(digit_material);

		// unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//=============================================================================================

	void NextLevelPage::draw_kill_count(ShaderProgramHandle digit_material)
	{
		// renders numbers char by char from right to left
		u32 display_count = enemy_count;

		vec2 position = vec2(0.75f, 0.3f);
		vec2 posDif = vec2(-0.06f, 0.0f);
		vec2 scale = vec2(0.0277f, 0.066f);

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager["ui_font"];

		do //DRAW TOTAL ENEMY COUNT
		{
			glActiveTexture(GL_TEXTURE0);

			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = position;
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = display_count % 10;
			digit += 48; // + '0'

			//setting shader
			digit_material.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_material.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_material.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_material.set_uniform(shaders::ui_text::HEIGHT, 16);
			digit_material.set_uniform(shaders::ui_text::WIDTH, 8);
			digit_material.set_uniform(shaders::ui_text::HOVER, false);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			display_count = display_count / 10;
			position += posDif;

		} while (display_count > 0);

		// DRAW SEPERATING '/'
		{
			glActiveTexture(GL_TEXTURE0);

			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = position;
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = (int)'/';

			// setting shader uniforms
			digit_material.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_material.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_material.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_material.set_uniform(shaders::ui_text::HEIGHT, 16);
			digit_material.set_uniform(shaders::ui_text::WIDTH, 8);
			digit_material.set_uniform(shaders::ui_text::HOVER, false);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			display_count = display_count / 10;
			position += posDif;

			display_count = kill_count;
		}

		do //DRAW KILL COUNT
		{
			glActiveTexture(GL_TEXTURE0);

			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = position;
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = display_count % 10;
			digit += 48; // + '0'

			//setting shader uniforms
			digit_material.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_material.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_material.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_material.set_uniform(shaders::ui_text::HEIGHT, 16);
			digit_material.set_uniform(shaders::ui_text::WIDTH, 8);
			digit_material.set_uniform(shaders::ui_text::HOVER, false);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			display_count = display_count / 10;
			position += posDif;

		} while (display_count > 0);
	}

	//=============================================================================================

	void MainMenuPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
	{
		//update buttons and determine which (one) button can be pressed based on mouse position

		UiButton* hover_over_button = nullptr;

		new_game_button->set_hover(false);
		options_button->set_hover(false);
		load_button->set_hover(false);
		quit_button->set_hover(false);

		if (new_game_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = new_game_button;
		}
		else if (options_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = options_button;
		}
		else if (load_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = load_button;
		}
		else if (quit_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = quit_button;
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}
	}

	//==============================================================================================
	void NewGamePage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
	{
		//update buttons and determine which (one) button can be pressed based on mouse position
		UiButton* hover_over_button = nullptr;

		go_back_button->set_hover(false);
		level_1_button->set_hover(false);
		level_2_button->set_hover(false);
		level_3_button->set_hover(false);

		if (level_1_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = level_1_button;
		}
		else if (level_2_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = level_2_button;
		}
		else if (level_3_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = level_3_button;
		}
		else if (go_back_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = go_back_button;
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}
	}

	//==============================================================================================
	void MainMenuPage::draw(ShaderProgramHandle button_material, GLuint VAO)
	{
		//shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		new_game_button->draw(button_material);
		options_button->draw(button_material);
		load_button->draw(button_material);
		//save_button->draw(button_material);
		quit_button->draw(button_material);
		nuclidean_text->draw(button_material);

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//=============================================================================================

	void NextLevelPage::next_level_func()
	{
		get_engine().send_event(ModuleEvent
			{
				.type = ModuleEventType::next_level_requested
			});
	}

	//=============================================================================================

	void NextLevelPage::menu_func()
	{
		get_engine().send_event(ModuleEvent
			{
				.type = ModuleEventType::main_menu_requested
			});
	}

	//==============================================================================================

	void MainMenuPage::new_game_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(NEW_GAME);
	}

	//==============================================================================================

	void MainMenuPage::options_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(OPTIONS);
	}

	//================================================================================================

	void MainMenuPage::load_game_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->update_saves();
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(LOAD);
	}

	//==============================================================================================

	void MainMenuPage::save_game_func()
	{
	}

	//================================================================================================

	void MainMenuPage::quit_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(QUIT);
	}

	//============================================================================================

	NewGamePage::NewGamePage()
	{
		go_back_button = new UiButton("ui_back", vec2(0.0f, 0.3f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::go_back, this));
		level_1_button = new UiButton("ui_level1", vec2(0.0f, 0.15f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_1_func, this));
		level_2_button = new UiButton("ui_level2", vec2(0.0f, 0.0f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_2_func, this));
		level_3_button = new UiButton("ui_level3", vec2(0.0f, -0.15f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_3_func, this));
	}

	//==============================================================================================

	NewGamePage::~NewGamePage()
	{
		delete level_1_button;
		delete level_2_button;
		delete level_3_button;
		delete go_back_button;
	}

	//==============================================================================================

	void NewGamePage::draw(ShaderProgramHandle button_material, GLuint VAO)
	{
		//shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		level_1_button->draw(button_material);
		level_2_button->draw(button_material);
		level_3_button->draw(button_material);
		go_back_button->draw(button_material);

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//==============================================================================================
	static void level_func(LevelName lvl)
	{
		get_engine().send_event(ModuleEvent
			{
				.type = ModuleEventType::new_game_level_requested,
				.new_game = EventNewGame{.level = lvl},
			});
	}

	//==============================================================================================
	void NewGamePage::level_1_func()
	{
		level_func(Levels::LEVEL_1);
	}

	//==============================================================================================
	void NewGamePage::level_2_func()
	{
		level_func(Levels::LEVEL_2);
	}

	//==============================================================================================
	void NewGamePage::level_3_func()
	{
		level_func(Levels::LEVEL_3);
	}

	//============================================================================================
	void OptionsPage::draw([[maybe_unused]] ShaderProgramHandle button_material, [[maybe_unused]] GLuint VAO)
	{
		//shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		sound_text->draw(button_material);
		music_text->draw(button_material);
		sensitivity_text->draw(button_material);
		crosshair_text->draw(button_material);
		shadow_text->draw(button_material);

		if (isWindowed)
		{
			windowed_button->draw(button_material);
		}
		else
		{
			fullscreen_button->draw(button_material);
		}

		if (isShadows)
		{
			shadow_on_button->draw(button_material);
		}
		else
		{
			shadow_off_button->draw(button_material);
		}

		sound_volume_less->draw(button_material);
		sound_volume_more->draw(button_material);

		music_volume_less->draw(button_material);
		music_volume_more->draw(button_material);

		sensitivity_more->draw(button_material);
		sensitivity_less->draw(button_material);

		crosshair_less->draw(button_material);
		crosshair_more->draw(button_material);

		go_back_button->draw(button_material);

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		// rendering of settings values
		// shader init
		digit_shader.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// rendering itself
		std::vector<vec2> positions = { vec2(0.4f, 0.30f), vec2(0.4f, 0.15f), vec2(0.4f, 0.0f) };
		std::vector<int> steps = { soundStep, musicStep, sensitivityStep - 1 }; //sensitivity step is 1 - 10, but we can draw only 0 - 9

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager["ui_font"];

		// draw numbers in the menu
		for (size_t i = 0; i < positions.size(); i++)
		{
			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = positions[i];
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(0.033f, 0.066f, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = steps[i] + 48;

			//set uniforms
			digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_shader.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_shader.set_uniform(shaders::ui_text::HEIGHT, 16);
			digit_shader.set_uniform(shaders::ui_text::WIDTH, 8);
			digit_shader.set_uniform(shaders::ui_text::HOVER, false);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		//Draw crosshair in menu
		// vec2(0.4f, -0.15f)
		const TextureHandle& texture_c = manager["ui_crosshair"];

		glm::mat4 trans_mat = glm::mat4(1.0f);
		vec2 translate = vec2(0.4f, -0.15f);
		trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
		trans_mat = glm::scale(trans_mat, glm::vec3(0.04f, 0.066f, 1));

		const glm::mat4 final_trans = trans_mat;

		// set uniforms
		digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
		digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture_c.get_atlas().get_size());
		digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture_c.get_pos());
		digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture_c.get_size());
		digit_shader.set_uniform(shaders::ui_text::CHARACTER, crosshairStep + 11);
		digit_shader.set_uniform(shaders::ui_text::HEIGHT, 2);
		digit_shader.set_uniform(shaders::ui_text::WIDTH, 11);
		digit_shader.set_uniform(shaders::ui_text::HOVER, false);

		glBindTexture(GL_TEXTURE_2D, texture_c.get_atlas().handle);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//=============================================================================================

	void OptionsPage::laod_settings()
	{
		std::ifstream f("settings.cfg");
		if (!f.is_open())
		{
			// we skip loading settings and use base values
			nc_warn("Failed to load settings");
			return;
		}
		auto data = nlohmann::json::parse(f);

		try
		{
			for (s32 sound_setting : data["sound"])
			{
				if (sound_setting < 0)
				{
					sound_setting = 0;
				}
				soundStep = min(9, sound_setting);
				get_engine().get_module<SoundSystem>().set_sound_volume(soundStep);
			}

			for (s32 music_setting : data["music"])
			{
				if (music_setting < 0)
				{
					music_setting = 0;
				}
				musicStep = min(9, music_setting);
				get_engine().get_module<SoundSystem>().set_music_volume(musicStep);
			}

			for (s32 sensitivity_setting : data["sensitivity"])
			{
				if (sensitivity_setting < 1)
				{
					sensitivity_setting = 1;
				}
				sensitivityStep = min(10, sensitivity_setting);
				get_engine().get_module<InputSystem>().set_sensitivity(sensitivityStep);
			}

			for (s32 crosshair_setting : data["crosshair"])
			{
				if (crosshair_setting < 0)
				{
					crosshair_setting = 0;
				}
				crosshairStep = min(9, crosshair_setting);
				get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshairStep);
			}

			for (bool fullscreen_setting : data["fullscreen"])
			{
				if (fullscreen_setting)
				{
					set_fullscreen();
				}
			}

			for (bool shadow_setting : data["shadows"])
			{
				if (shadow_setting)
				{
					shadow_on();
				}
				else
				{
					shadow_off();
				}
			}
		}
		catch (int) {}
	}

	//=============================================================================================
	void OptionsPage::save_settings()
	{
		std::ofstream f("settings.cfg");
		if (!f.is_open())
		{
			nc_warn("Failed to write settings");
		}
		nlohmann::json data;

		data["sound"] = soundStep;
		data["music"] = musicStep;
		data["sensitivity"] = sensitivityStep;
		data["crosshair"] = crosshairStep;
		data["fullscreen"] = !isWindowed;
		data["shadows"] = isShadows;

		f << data;
		f.close();
	}

	//=============================================================================================

	void OptionsPage::set_windowed()
	{
		SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
		SDL_SetWindowFullscreen(window, 0);
		isWindowed = true;
	}

	//=============================================================================================

	void OptionsPage::set_fullscreen()
	{
		SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		isWindowed = false;
	}

	//=============================================================================================

	void OptionsPage::set_sensitivity_less()
	{
		sensitivityStep = max(1, sensitivityStep - 1);
		get_engine().get_module<InputSystem>().set_sensitivity(sensitivityStep);
	}

	//=============================================================================================

	void OptionsPage::set_sensitivity_more()
	{
		sensitivityStep = min(10, sensitivityStep + 1);
		get_engine().get_module<InputSystem>().set_sensitivity(sensitivityStep);
	}

	//=============================================================================================

	void OptionsPage::set_sound_less()
	{
		soundStep = max(0, soundStep - 1);
		get_engine().get_module<SoundSystem>().set_sound_volume(soundStep);
	}

	//=============================================================================================

	void OptionsPage::set_sound_more()
	{
		soundStep = min(9, soundStep + 1);
		get_engine().get_module<SoundSystem>().set_sound_volume(soundStep);
	}

	//=============================================================================================

	void OptionsPage::set_music_less()
	{
		musicStep = max(0, musicStep - 1);
		get_engine().get_module<SoundSystem>().set_music_volume(musicStep);
	}

	//=============================================================================================

	void OptionsPage::set_music_more()
	{
		musicStep = min(9, musicStep + 1);
		get_engine().get_module<SoundSystem>().set_music_volume(musicStep);
	}

	//=============================================================================================

	void OptionsPage::set_crosshair_less()
	{
		crosshairStep = (crosshairStep - 1);
		if (crosshairStep < 0)
		{
			crosshairStep = 9;
		}
		get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshairStep);
	}

	//=============================================================================================

	void OptionsPage::set_crosshair_more()
	{
		crosshairStep = (crosshairStep + 1) % 10;
		get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshairStep);
	}

	void OptionsPage::shadow_on()
	{
		isShadows = true;
	}

	void OptionsPage::shadow_off()
	{
		isShadows = false;
	}

	//=============================================================================================

	OptionsPage::OptionsPage() :
		digit_shader(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
	{
		sound_text = new UiButton("ui_sound", vec2(-0.3f, 0.30f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		music_text = new UiButton("ui_music", vec2(-0.3f, 0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		sensitivity_text = new UiButton("ui_sensitivity", vec2(-0.3f, 0.0f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		crosshair_text = new UiButton("ui_crosshair_text", vec2(-0.3f, -0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		shadow_text = new UiButton("ui_shadows", vec2(-0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		shadow_on_button = new UiButton("ui_on", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::shadow_off, this));
		shadow_off_button = new UiButton("ui_off", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::shadow_on, this));
		fullscreen_button = new UiButton("ui_fullscreen", vec2(0.3f, -0.45f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_windowed, this));
		windowed_button = new UiButton("ui_windowed", vec2(0.3f, -0.45f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_fullscreen, this));

		sound_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_less, this));
		sound_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_more, this));
		music_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_less, this));
		music_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_more, this));
		sensitivity_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_less, this));
		sensitivity_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_more, this));
		crosshair_less = new UiButton("ui_arrow_left", vec2(0.3f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_less, this));
		crosshair_more = new UiButton("ui_arrow_right", vec2(0.5f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_more, this));

		go_back_button = new UiButton("ui_back", vec2(-0.3f, 0.45f), vec2(0.28f, 0.066f), std::bind(&OptionsPage::go_back, this));
	}

	//=============================================================================================

	OptionsPage::~OptionsPage()
	{
		delete sound_text;
		delete music_text;
		delete sensitivity_text;
		delete fullscreen_button;
		delete windowed_button;
		delete sound_volume_less;
		delete sound_volume_more;
		delete music_volume_less;
		delete music_volume_more;
		delete sensitivity_less;
		delete sensitivity_more;
		delete crosshair_less;
		delete crosshair_more;
		delete go_back_button;
	}

	//==============================================================================================
	void OptionsPage::update([[maybe_unused]] vec2 mouse_pos, [[maybe_unused]] u32 prev_mouse, [[maybe_unused]] u32 cur_mouse)
	{
		//update buttons and determine which (one) button can be pressed based on mouse position
		UiButton* hover_over_button = nullptr;

		fullscreen_button->set_hover(false);
		windowed_button->set_hover(false);

		sound_volume_less->set_hover(false);
		sound_volume_more->set_hover(false);

		music_volume_less->set_hover(false);
		music_volume_more->set_hover(false);

		sensitivity_less->set_hover(false);
		sensitivity_more->set_hover(false);

		crosshair_less->set_hover(false);
		crosshair_more->set_hover(false);

		go_back_button->set_hover(false);

		shadow_on_button->set_hover(false);
		shadow_off_button->set_hover(false);

		if (isWindowed)
		{
			if (windowed_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = windowed_button;
			}
		}
		else
		{
			if (fullscreen_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = fullscreen_button;
			}
		}

		if (isShadows)
		{
			if (shadow_on_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = shadow_on_button;
			}
		}
		else
		{
			if (shadow_off_button->is_point_in_rec(mouse_pos))
			{
				hover_over_button = shadow_off_button;
			}
		}


		if (sound_volume_less->is_point_in_rec(mouse_pos))
		{
			hover_over_button = sound_volume_less;
		}
		else if (sound_volume_more->is_point_in_rec(mouse_pos))
		{
			hover_over_button = sound_volume_more;
		}
		else if (music_volume_less->is_point_in_rec(mouse_pos))
		{
			hover_over_button = music_volume_less;
		}
		else if (music_volume_more->is_point_in_rec(mouse_pos))
		{
			hover_over_button = music_volume_more;
		}
		else if (sensitivity_less->is_point_in_rec(mouse_pos))
		{
			hover_over_button = sensitivity_less;
		}
		else if (sensitivity_more->is_point_in_rec(mouse_pos))
		{
			hover_over_button = sensitivity_more;
		}
		else if (crosshair_less->is_point_in_rec(mouse_pos))
		{
			hover_over_button = crosshair_less;
		}
		else if (crosshair_more->is_point_in_rec(mouse_pos))
		{
			hover_over_button = crosshair_more;
		}
		else if (go_back_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = go_back_button;
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}
	}

	//=============================================================================================

	LoadGamePage::LoadGamePage()
	{
		go_back_button = new UiButton("ui_back", vec2(0.0f, 0.6f), vec2(0.3f, 0.066f), std::bind(&LoadGamePage::go_back, this));
		page_up_button = new UiButton("ui_arrow_down", vec2(0.0f, -0.55f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_up, this));
		page_down_button = new UiButton("ui_arrow_up", vec2(0.0f, 0.45f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_down, this));
	}

	//=============================================================================================

	LoadGamePage::~LoadGamePage()
	{
		delete go_back_button;
		delete page_down_button;
		delete page_up_button;

		for (auto load_game_button : load_game_buttons)
		{
			delete load_game_button;
		}
		load_game_buttons.clear();
	}

	//=============================================================================================

	void LoadGamePage::update_saves()
	{
		//delete buttons
		for (auto load_game_button : load_game_buttons)
		{
			delete load_game_button;
		}
		load_game_buttons.clear();

		// get save db
		nc::GameSystem::SaveDatabase& save_db = get_engine().get_module<GameSystem>().get_save_game_db();

		// starting pos for first button and steps for next
		vec2 pos = vec2(-0.0f, 0.3f);
		vec2 stepPos = vec2(0.0f, -0.1f);
		vec2 scale = vec2(0.5f, 0.033f);

		// create buttons
		for (auto& save : save_db)
		{
			load_game_buttons.push_back(new UiLoadGameButton(save, pos, scale));
			pos += stepPos;
			if (pos.y < -0.41f)
			{
				pos = vec2(-0.0f, 0.3f);
			}
		}

		page = 0;
	}

	//==============================================================================================
	void LoadGamePage::update([[maybe_unused]] vec2 mouse_pos, [[maybe_unused]] u32 prev_mouse, [[maybe_unused]] u32 cur_mouse)
	{
		//update buttons and determine which (one) button can be pressed based on mouse position
		go_back_button->set_hover(false);
		page_down_button->set_hover(false);
		page_up_button->set_hover(false);
		for (auto& button : load_game_buttons)
		{
			button->set_hover(false);
		}

		UiButton* hover_over_button = nullptr;

		if (go_back_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = go_back_button;
		}
		else if (page_down_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = page_down_button;
		}
		else if (page_up_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = page_up_button;
		}

		for (size_t i = 0 + page * PAGE_SIZE;
			i < load_game_buttons.size() && i < (page + 1) * PAGE_SIZE;
			i++)
		{
			if (load_game_buttons[i]->is_point_in_rec(mouse_pos))
			{
				hover_over_button = load_game_buttons[i];
				break;
			}
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}

	}

	//==============================================================================================
	void LoadGamePage::draw([[maybe_unused]] ShaderProgramHandle button_material, ShaderProgramHandle digit_material, [[maybe_unused]] GLuint VAO)
	{
		//shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// render scrolling and go back buttons
		go_back_button->draw(button_material);
		page_down_button->draw(button_material);
		page_up_button->draw(button_material);

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		//shader init
		digit_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//render load game buttons
		for (size_t i = 0 + page * PAGE_SIZE;
			i < load_game_buttons.size() && i < (page + 1) * PAGE_SIZE;
			i++)
		{
			load_game_buttons[i]->draw(digit_material);
		}

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//=============================================================================================

	void LoadGamePage::go_back()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	//=============================================================================================
	void LoadGamePage::page_up()
	{
		page = min(page + 1, (s32)load_game_buttons.size() / PAGE_SIZE);
	}

	//=============================================================================================
	void LoadGamePage::page_down()
	{
		page = max(0, page - 1);
	}

	//=============================================================================================

	void OptionsPage::go_back()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	//=============================================================================================
	void NewGamePage::go_back()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	//==============================================================================================
	QuitGamePage::QuitGamePage()
	{
		yes_button = new UiButton("ui_yes", vec2(0.0f, 0.05f), vec2(0.3f, 0.066f), std::bind(&QuitGamePage::yes_func, this));
		no_button = new UiButton("ui_no", vec2(0.0f, -0.1f), vec2(0.3f, 0.066f), std::bind(&QuitGamePage::no_func, this));
	}

	//==============================================================================================
	QuitGamePage::~QuitGamePage()
	{
		delete yes_button;
		delete no_button;
	}

	//==============================================================================================
	void QuitGamePage::update([[maybe_unused]] vec2 mouse_pos, [[maybe_unused]] u32 prev_mouse, [[maybe_unused]] u32 cur_mouse)
	{
		//update buttons and determine which (one) button can be pressed based on mouse position
		UiButton* hover_over_button = nullptr;

		yes_button->set_hover(false);
		no_button->set_hover(false);

		if (yes_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = yes_button;
		}
		else if (no_button->is_point_in_rec(mouse_pos))
		{
			hover_over_button = no_button;
		}

		if (hover_over_button != nullptr)
		{
			hover_over_button->set_hover(true);

			if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
			{
				hover_over_button->on_click();
			}
		}
	}

	//==============================================================================================
	void QuitGamePage::draw([[maybe_unused]] ShaderProgramHandle button_material, [[maybe_unused]] GLuint VAO)
	{
		//shader init
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		yes_button->draw(button_material);
		no_button->draw(button_material);

		//unbind
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	//==============================================================================================
	void QuitGamePage::yes_func()
	{
		get_engine().request_quit();
	}

	//==============================================================================================
	void QuitGamePage::no_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	//===========================================================================================

	MainMenuPage::MainMenuPage()
	{
		const char* ng_text = "ui_new_game";
		const char* o_text = "ui_options";
		const char* lg_text = "ui_load_game";
		const char* q_text = "ui_quit";

		new_game_button = new UiButton(ng_text, vec2(0.0f, 0.15f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::new_game_func, this));
		options_button = new UiButton(o_text, vec2(0.0f, 0.0f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::options_func, this));
		load_button = new UiButton(lg_text, vec2(0.0f, -0.15f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::load_game_func, this));
		quit_button = new UiButton(q_text, vec2(0.0f, -0.3f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::quit_func, this));
		nuclidean_text = new UiButton("ui_nuclidean", vec2(0.0f, 0.5f), vec2(0.42f, 0.1f), std::bind(&MainMenuPage::quit_func, this));
	}

	//============================================================================================

	MainMenuPage::~MainMenuPage()
	{
		delete new_game_button;
		delete options_button;
		delete load_button;
		delete quit_button;
		delete nuclidean_text;
	}
}