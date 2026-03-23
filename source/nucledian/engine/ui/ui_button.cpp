#include <engine/ui/user_interface_module.h>

#include <engine/graphics/resources/res_lifetime.h>

#include <engine/ui/ui_button.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <engine/game/game_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/enemies/enemy.h>
#include <engine/game/game_helpers.h>
#include <engine/core/engine.h>
#include <engine/input/input_system.h>
#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>
#include <engine/core/module_event.h>
#include <engine/graphics/shaders/shaders.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <json/json.hpp>

namespace nc
{
	UiButton::UiButton()
	{
	}

	UiButton::UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func)
	{
		this->texture_name = texture_name;

		this->position = position;
		this->scale = scale;
		this->func = func;

	}

	//================================================================================================

	bool UiButton::is_point_in_rec(vec2 point)
	{
		vec2 start = position - scale;
		vec2 end = position + scale;

		if (point.x < start.x || point.x > end.x)
		{
			return false;
		}

		if (point.y < start.y || point.y > end.y)
		{
			return false;
		}

		return true;
	}

	//===================================================================================================

	vec2 UiButton::get_position()
	{
		return position;
	}

	//======================================================================================================

	vec2 UiButton::get_scale()
	{
		return scale;
	}

	//==============================================================================================

	void UiButton::set_hover(bool hover)
	{
		isHover = hover;
	}

	//==============================================================================================

	void UiButton::on_click()
	{
		SoundSystem::get().play_oneshot(Sounds::ui_click, 1.0f, SoundLayers::ui);
		func();
	}

	//====================================================================================================

	void UiButton::draw([[maybe_unused]] const ShaderProgramHandle button_material)
	{
		button_material.use();

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager[texture_name];

		glActiveTexture(GL_TEXTURE0);

		glm::mat4 trans_mat = glm::mat4(1.0f);
		vec2 translate = position;
		trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
		trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

		const glm::mat4 final_trans = trans_mat;

		button_material.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
		button_material.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas().get_size());
		button_material.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
		button_material.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());
		button_material.set_uniform(shaders::ui_button::HOVER, isHover);

		glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	//============================================================================================

	MenuManager::MenuManager() :
		button_material(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE),
		digit_material(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
	{

		main_menu_page = new MainMenuPage();
		options_page = new OptionsPage();
		load_game_page = new LoadGamePage();
		new_game_page = new NewGamePage();
		quit_game_page = new QuitGamePage();
		next_level_page = new NextLevelPage();

		vec2 vertices[] = { vec2(-1.0f, 1.0f), vec2(0.0f, 0.015f),
			vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f - 0.015f),
			vec2(1.0f, 1.0f), vec2(1.0f, 0.015f),
			vec2(1.0f, -1.0f), vec2(1.0f, 1.0f - 0.015f) };

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);

		glGenVertexArrays(1, &VAO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);
		// position
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glEnableVertexAttribArray(0);
		// uvs
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	//============================================================================================

	void MenuManager::post_init()
	{
		options_page->laod_settings();
	}

	MenuManager::~MenuManager()
	{
		delete main_menu_page;
		delete options_page;
		delete load_game_page;
		delete new_game_page;
		delete next_level_page;

		glDeleteBuffers(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	//==============================================================================================

	void MenuManager::set_page(MenuPages page)
	{
		current_page = page;
	}

	//=============================================================================================

	void MenuManager::on_exit()
	{
		options_page->save_settings();
	}

	//=============================================================================================

	vec2 MenuManager::get_normalized_mouse_pos()
	{
		int mouse_x, mouse_y;

		uint32 state = SDL_GetMouseState(&mouse_x, &mouse_y);

		prev_mousestate = cur_mousestate;
		cur_mousestate = state;

		vec2 dimensions = get_engine().get_module<GraphicsSystem>().get_window_size();

		f32 width = dimensions.x;
		f32 height = dimensions.y;

		f32 half_width = width / 2.0f;
		f32 half_height = height / 2.0f;

		//normalize between <0, 2>, then minus one to move it to <-1, 1>
		f32 norm_x = mouse_x / half_width - 1.0f;
		f32 norm_y = mouse_y / half_height - 1.0f;

		return vec2(norm_x, -norm_y);
	}

	//=============================================================================================
  void MenuManager::set_transition_screen([[maybe_unused]]bool enabled)
  {
		isTransition = enabled;
		if (isTransition)
		{
			[[maybe_unused]] f64 time = GameHelpers::get().get_time_since_start();
			u32 enemies = get_engine().get_module<GameSystem>().get_enemy_count();
			u32 kills = get_engine().get_module<GameSystem>().get_kill_count();
			next_level_page->set_kill_stats(enemies, kills);
		}
  }

	//=============================================================================================

	void MenuManager::set_visible(bool visibility)
	{
    if (visible != visibility)
    {
      namespace EventType = ModuleEventType;

      get_engine().send_event(ModuleEvent
      {
        .type = visibility ? EventType::menu_opened : EventType::menu_closed
      });
    }

		visible = visibility;
		current_page = MAIN;
	}

	//===============================================================================================

	void MenuManager::update()
	{
		if (isTransition)
		{
			vec2 mouse_pos = get_normalized_mouse_pos();
			next_level_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			return;
		}


		prev_esc_pressed = cur_esc_pressed;
		cur_esc_pressed = false;

		int size;
		const Uint8* keyboard = SDL_GetKeyboardState(&size);
		if (keyboard[SDL_SCANCODE_ESCAPE])
		{
			cur_esc_pressed = true;
		}

    bool pressed = cur_esc_pressed && !prev_esc_pressed;
    bool locked  = get_engine().is_menu_locked_visible();

    if (pressed)
    {
      if (current_page != MenuPages::MAIN)
      {
        // Go back to menu
        set_page(MenuPages::MAIN);
      }
      else if (!locked)
      {
        // Change visibility
        set_visible(!visible);
      }
    }

		if (!visible) {
			return;
		}

		vec2 mouse_pos = get_normalized_mouse_pos();

		switch (current_page)
		{
		case nc::MAIN:
			main_menu_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			break;
		case nc::NEW_GAME:
			new_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			break;
		case nc::OPTIONS:
			options_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			break;
		case nc::LOAD:
			load_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			break;
		case nc::SAVE:
			break;
		case nc::QUIT:
			quit_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
			break;
		default:
			break;
		}
	}

	//============================================================================================

	void MenuManager::draw()
	{
		if (isTransition)
		{
			next_level_page->draw(button_material, digit_material, VAO);
			draw_cursor();
			return;
		}

		if (!visible)
		{
			return;
		}

		switch (current_page)
		{
		case nc::MAIN:
			main_menu_page->draw(button_material, VAO);
			break;
		case nc::NEW_GAME:
			new_game_page->draw(button_material, VAO);
			break;
		case nc::OPTIONS:
			options_page->draw(button_material, VAO);
			break;
		case nc::LOAD:
			load_game_page->draw(button_material, digit_material, VAO);
			break;
		case nc::SAVE:
			break;
		case nc::QUIT:
			quit_game_page->draw(button_material, VAO);
			break;
		default:
			break;
		}

		draw_cursor();
	}

	//==============================================================================================

	void MenuManager::draw_cursor()
	{
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		vec2 pos = get_normalized_mouse_pos();

		const char* cursor_tex = "ui_cursor";

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager[cursor_tex];

		glActiveTexture(GL_TEXTURE0);

		glm::mat4 trans_mat = glm::mat4(1.0f);
		vec2 translate = pos + vec2(0.015, -0.02);
		trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
		trans_mat = glm::scale(trans_mat, glm::vec3(0.015f, 0.02f, 1));

		const glm::mat4 final_trans = trans_mat;

		button_material.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
		button_material.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas().get_size());
		button_material.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
		button_material.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());
		button_material.set_uniform(shaders::ui_button::HOVER, false);

		glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
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

	//=============================================================================================

	NextLevelPage::NextLevelPage()
	{
		next_level_button = new UiButton("ui_next_level", vec2(0.0f, -0.8f), vec2(0.3f, 0.066f), std::bind(&NextLevelPage::next_level_func, this));
		menu_button = new UiButton("ui_menu", vec2(0.0f, -0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::menu_func, this));
		kills_text = new UiButton("ui_kills", vec2(-0.3f, 0.3f), vec2(0.30f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		level_text = new UiButton("ui_level", vec2(0.0f, 0.8f), vec2(0.15f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		demo_text = new UiButton("ui_demo", vec2(0.0f, 0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
		completed_text = new UiButton("ui_completed", vec2(0.0f, 0.6f), vec2(0.253f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
	}

	NextLevelPage::~NextLevelPage()
	{
		delete next_level_button;
		delete kills_text;
		delete demo_text;
		delete level_text;
		delete completed_text;
		delete menu_button;
	}

	void NextLevelPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
	{
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

	void NextLevelPage::draw(ShaderProgramHandle button_material, ShaderProgramHandle digit_material, GLuint VAO)
	{
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
		

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		draw_stats(digit_material, VAO);
	}

	void NextLevelPage::set_kill_stats(u32 enemies, u32 kills)
	{
		enemy_count = enemies;
		kill_count = kills;
	}

	void NextLevelPage::draw_stats(ShaderProgramHandle digit_material, GLuint VAO)
	{
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

	void NextLevelPage::draw_kill_count(ShaderProgramHandle digit_material)
	{
		u32 display_count = enemy_count;

		vec2 position = vec2(0.75f, 0.3f);
		vec2 posDif = vec2(-0.06f, 0.0f);
		vec2 scale = vec2(0.0277f, 0.066f);

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager["ui_font"];

		do //DRAW ENEMY COUNT
		{
			glActiveTexture(GL_TEXTURE0);

			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = position;
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = display_count % 10;
			digit += 48;

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

			int digit = 47; // = '/'

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
			digit += 48;

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

	void MainMenuPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
	{
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

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	void NextLevelPage::next_level_func()
	{
		get_engine().send_event(ModuleEvent
			{
				.type = ModuleEventType::next_level_requested
			});
	}

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

		if (isWindowed)
		{
			windowed_button->draw(button_material);
		}
		else
		{
			fullscreen_button->draw(button_material);
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

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
		
		// rendering of values

		digit_shader.use();

		std::vector<vec2> positions = { vec2(0.4f, 0.30f), vec2(0.4f, 0.15f), vec2(0.4f, 0.0f)};
		std::vector<int> steps = { soundStep, musicStep, sensitivityStep - 1}; //sensitivity step is 1 - 10, but we can draw only 0 - 9

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	void OptionsPage::laod_settings()
	{
		std::ifstream f("settings.cfg");
		if (!f.is_open())
		{
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
		}
		catch(int){}
	}

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

		f << data;
		f.close();
	}

	void OptionsPage::set_windowed()
	{
		SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
		SDL_SetWindowFullscreen(window, 0);
		isWindowed = true;
	}

	void OptionsPage::set_fullscreen()
	{
		SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		isWindowed = false;
	}

	void OptionsPage::set_sensitivity_less()
	{
		sensitivityStep = max(1, sensitivityStep - 1);
		get_engine().get_module<InputSystem>().set_sensitivity(sensitivityStep);
	}

	void OptionsPage::set_sensitivity_more()
	{
		sensitivityStep = min(10, sensitivityStep + 1);
		get_engine().get_module<InputSystem>().set_sensitivity(sensitivityStep);
	}

	void OptionsPage::set_sound_less()
	{
		soundStep = max(0, soundStep - 1);
		get_engine().get_module<SoundSystem>().set_sound_volume(soundStep);
	}

	void OptionsPage::set_sound_more()
	{
		soundStep = min(9, soundStep + 1);
		get_engine().get_module<SoundSystem>().set_sound_volume(soundStep);
	}

	void OptionsPage::set_music_less()
	{
		musicStep = max(0, musicStep - 1);
		get_engine().get_module<SoundSystem>().set_music_volume(musicStep);
	}

	void OptionsPage::set_music_more()
	{
		musicStep = min(9, musicStep + 1);
		get_engine().get_module<SoundSystem>().set_music_volume(musicStep);
	}

	void OptionsPage::set_crosshair_less()
	{
		crosshairStep = (crosshairStep - 1);
		if (crosshairStep < 0)
		{
			crosshairStep = 9;
		}
		get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshairStep);
	}

	void OptionsPage::set_crosshair_more()
	{
		crosshairStep = (crosshairStep + 1) % 10;
		get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshairStep);
	}

	OptionsPage::OptionsPage() :
		digit_shader(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
	{
		sound_text = new UiButton("ui_sound", vec2(-0.3f, 0.30f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		music_text = new UiButton("ui_music", vec2(-0.3f, 0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		sensitivity_text = new UiButton("ui_sensitivity", vec2(-0.3f, 0.0f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		crosshair_text = new UiButton("ui_crosshair_text", vec2(-0.3f, -0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
		fullscreen_button = new UiButton("ui_fullscreen", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_windowed, this));
		windowed_button = new UiButton("ui_windowed", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_fullscreen, this));

		sound_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_less, this));
		sound_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_more, this));
		music_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_less, this));
		music_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_more, this));
		sensitivity_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_less, this));
		sensitivity_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_more, this));
		crosshair_less = new UiButton("ui_arrow_left", vec2(0.3f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_less, this));
		crosshair_more = new UiButton("ui_arrow_right", vec2(0.5f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_more, this));

		go_back_button = new UiButton("ui_back", vec2(-0.3f, 0.45f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::go_back, this));
	}

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

	LoadGamePage::LoadGamePage()
	{
		go_back_button = new UiButton("ui_back", vec2(0.0f, 0.6f), vec2(0.3f, 0.066f), std::bind(&LoadGamePage::go_back, this));
		page_up_button = new UiButton("ui_arrow_down", vec2(0.0f, -0.55f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_up, this));
		page_down_button = new UiButton("ui_arrow_up", vec2(0.0f, 0.45f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_down, this));
	}

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

	void LoadGamePage::update_saves()
	{
		for (auto load_game_button : load_game_buttons)
		{
			delete load_game_button;
		}
		load_game_buttons.clear();
		nc::GameSystem::SaveDatabase& save_db = get_engine().get_module<GameSystem>().get_save_game_db();

		vec2 pos = vec2(-0.0f, 0.3f);
		vec2 stepPos = vec2(0.0f, -0.1f);
		vec2 scale = vec2(0.5f, 0.033f);

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

	void MenuManager::update_saves()
	{
		load_game_page->update_saves();
	}

	//==============================================================================================
	void LoadGamePage::update([[maybe_unused]] vec2 mouse_pos, [[maybe_unused]] u32 prev_mouse, [[maybe_unused]] u32 cur_mouse)
	{
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
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		go_back_button->draw(button_material);
		page_down_button->draw(button_material);
		page_up_button->draw(button_material);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		digit_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (size_t i = 0 + page * PAGE_SIZE;
			i < load_game_buttons.size() && i < (page + 1) * PAGE_SIZE; 
			i++)
		{
			load_game_buttons[i]->draw(digit_material);
		}

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	void LoadGamePage::go_back()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	void LoadGamePage::page_up()
	{
		page = min(page + 1, (s32)load_game_buttons.size() / PAGE_SIZE);
	}

	void LoadGamePage::page_down()
	{
		page = max(0, page - 1);
	}

	void OptionsPage::go_back()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

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
		button_material.use();

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		yes_button->draw(button_material);
		no_button->draw(button_material);

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
		//get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->on_exit();

		get_engine().request_quit();
	}

	//==============================================================================================
	void QuitGamePage::no_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}

	//==============================================================================================

	UiLoadGameButton::UiLoadGameButton(nc::GameSystem::SaveDbEntry& save_entry, vec2 position, vec2 scale) :
		save(save_entry)
	{
		this->position = position;
		this->scale = scale;
		
	}

	void UiLoadGameButton::on_click()
	{
		get_engine().get_module<GameSystem>().load_game(save.data);
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_visible(false);
		
	}

	//==============================================================================================

	void UiLoadGameButton::draw(ShaderProgramHandle digit_material)
	{
		nc::SaveGameData::SaveTime time = save.data.time;

		std::string text = std::format("{:%Y-%m-%d %H:%M}", time);

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager["ui_font"];

		vec2 final_pos = position - vec2(0.5f, 0.0f);
		vec2 step = vec2(0.033f, 0.0f);

		for (size_t c = 0; c < text.size(); c++)
		{
			glActiveTexture(GL_TEXTURE0);

			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = final_pos;
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(0.0165f, 0.033f, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = int(text[c]);

			digit_material.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_material.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_material.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_material.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_material.set_uniform(shaders::ui_text::HEIGHT, 16);
			digit_material.set_uniform(shaders::ui_text::WIDTH, 8);
			digit_material.set_uniform(shaders::ui_text::HOVER, isHover);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			final_pos += step;
		}
	}
}