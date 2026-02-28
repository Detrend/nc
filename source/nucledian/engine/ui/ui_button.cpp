#include <engine/ui/user_interface_module.h>

#include <engine/graphics/resources/res_lifetime.h>

#include <engine/ui/ui_button.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <engine/core/engine.h>
#include <engine/game/game_system.h>
#include <engine/input/input_system.h>
#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>
#include <engine/core/module_event.h>

namespace nc
{
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
		SoundSystem::get().play(Sounds::ui_click, 1.0f);
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
		button_material(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE)
	{

		main_menu_page = new MainMenuPage();
		options_page = new OptionsPage();
		load_game_page = new LoadGamePage();
		new_game_page = new NewGamePage();
		quit_game_page = new QuitGamePage();

		vec2 vertices[] = { vec2(-1, 1), vec2(0, 0),
				vec2(-1, -1), vec2(0, 1),
				vec2(1, 1), vec2(1, 0),
				vec2(1, -1), vec2(1, 1) };

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

	MenuManager::~MenuManager()
	{
		delete main_menu_page;
		delete options_page;
		delete load_game_page;
		delete new_game_page;

		glDeleteBuffers(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}

	//==============================================================================================

	void MenuManager::set_page(MenuPages page)
	{
		current_page = page;
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

		new_game_button = new UiButton(ng_text, vec2(0.0f, 0.15f), vec2(0.45f, 0.1f), std::bind(&MainMenuPage::new_game_func, this));
		options_button = new UiButton(o_text, vec2(0.0f, -0.1f), vec2(0.45f, 0.1f), std::bind(&MainMenuPage::options_func, this));
		load_button = new UiButton(lg_text, vec2(0.0f, -0.35f), vec2(0.45f, 0.1f), std::bind(&MainMenuPage::load_game_func, this));
		quit_button = new UiButton(q_text, vec2(0.0f, -0.6f), vec2(0.45f, 0.1f), std::bind(&MainMenuPage::quit_func, this));
	}

	//============================================================================================

	MainMenuPage::~MainMenuPage()
	{
		delete new_game_button;
		delete options_button;
		delete load_button;
		delete quit_button;
	}

	//=============================================================================================

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


		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
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
		level_1_button = new UiButton("ui_level1", vec2(0.0f, 0.15f), vec2(0.45f, 0.1f), std::bind(&NewGamePage::level_1_func, this));
		level_2_button = new UiButton("ui_level2", vec2(0.0f, -0.1f), vec2(0.45f, 0.1f), std::bind(&NewGamePage::level_2_func, this));
		level_3_button = new UiButton("ui_level3", vec2(0.0f, -0.35f), vec2(0.45f, 0.1f), std::bind(&NewGamePage::level_3_func, this));
	}

	//==============================================================================================
	NewGamePage::~NewGamePage()
	{
		delete level_1_button;
		delete level_2_button;
		delete level_3_button;
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

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
		
		// rendering of values

		digit_shader.use();

		std::vector<vec2> positions = { vec2(0.4f, 0.40f), vec2(0.4f, 0.15f), vec2(0.4f, -0.1f), vec2(0.4f, -0.35f) };
		std::vector<int> steps = { soundStep, musicStep, sensitivityStep - 1, crosshairStep }; //sensitivity step is 1 - 10, but we can draw only 0 - 9

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const TextureManager& manager = TextureManager::get();
		const TextureHandle& texture = manager["ui_font"];

		for (size_t i = 0; i < positions.size(); i++)
		{
			glm::mat4 trans_mat = glm::mat4(1.0f);
			vec2 translate = positions[i];
			trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
			trans_mat = glm::scale(trans_mat, glm::vec3(0.05f, 0.1f, 1));

			const glm::mat4 final_trans = trans_mat;

			int digit = steps[i] + 48;

			digit_shader.set_uniform(shaders::ui_text::TRANSFORM, final_trans);
			digit_shader.set_uniform(shaders::ui_text::ATLAS_SIZE, texture.get_atlas().get_size());
			digit_shader.set_uniform(shaders::ui_text::TEXTURE_POS, texture.get_pos());
			digit_shader.set_uniform(shaders::ui_text::TEXTURE_SIZE, texture.get_size());
			digit_shader.set_uniform(shaders::ui_text::CHARACTER, digit);
			digit_shader.set_uniform(shaders::ui_text::HEIGHT, 16.0f);
			digit_shader.set_uniform(shaders::ui_text::WIDTH, 8.0f);

			glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
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
		crosshairStep = (crosshairStep - 1) % 10;
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
		sound_text = new UiButton("ui_sound", vec2(-0.3f, 0.40f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::do_nothing, this));
		music_text = new UiButton("ui_music", vec2(-0.3f, 0.15f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::do_nothing, this));
		sensitivity_text = new UiButton("ui_sensitivity", vec2(-0.3f, -0.1f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::do_nothing, this));
		crosshair_text = new UiButton("ui_crosshair_text", vec2(-0.3f, -0.35f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::do_nothing, this));
		fullscreen_button = new UiButton("ui_fullscreen", vec2(0.3f, -0.6f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::set_windowed, this));
		windowed_button = new UiButton("ui_windowed", vec2(0.3f, -0.6f), vec2(0.45f, 0.1f), std::bind(&OptionsPage::set_fullscreen, this));

		sound_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.40f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_sound_less, this));
		sound_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.40f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_sound_more, this));
		music_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.15f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_music_less, this));
		music_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.15f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_music_more, this));
		sensitivity_less = new UiButton("ui_arrow_left", vec2(0.3f, -0.1f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_sensitivity_less, this));
		sensitivity_more = new UiButton("ui_arrow_right", vec2(0.5f, -0.1f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_sensitivity_more, this));
		crosshair_less = new UiButton("ui_arrow_left", vec2(0.3f, -0.35f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_crosshair_less, this));
		crosshair_more = new UiButton("ui_arrow_right", vec2(0.5f, -0.35f), vec2(0.05f, 0.1f), std::bind(&OptionsPage::set_crosshair_more, this));
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
	void LoadGamePage::update([[maybe_unused]] vec2 mouse_pos, [[maybe_unused]] u32 prev_mouse, [[maybe_unused]] u32 cur_mouse)
	{
	}

	//==============================================================================================
	void LoadGamePage::draw([[maybe_unused]] ShaderProgramHandle button_material, [[maybe_unused]] GLuint VAO)
	{

	}

	//==============================================================================================
	QuitGamePage::QuitGamePage()
	{
		yes_button = new UiButton("ui_yes", vec2(0.0f, 0.15f), vec2(0.45f, 0.1f), std::bind(&QuitGamePage::yes_func, this));
		no_button = new UiButton("ui_no", vec2(0.0f, -0.1f), vec2(0.45f, 0.1f), std::bind(&QuitGamePage::no_func, this));
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
		get_engine().request_quit();
	}

	//==============================================================================================
	void QuitGamePage::no_func()
	{
		get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MAIN);
	}
}