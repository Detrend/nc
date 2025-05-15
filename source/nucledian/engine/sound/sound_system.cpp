// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/sound/sound_system.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/gizmo.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <engine/input/input_system.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/player/thing_system.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>
#include <SDL2/include/SDL_mixer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_stdlib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

// Remove this after logging is added!
#include <iostream>

namespace nc
{

    //==============================================================================
    SoundSystem::SoundSystem()
    {
    }

    //==============================================================================
    EngineModuleId SoundSystem::get_module_id()
    {
        return EngineModule::sound_system;
    }

    //==============================================================================
    bool SoundSystem::init()
    {
        //Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        {
            nc_expect(false, "SDL_mixer could not initialize! SDL_mixer Error: {0}\n", Mix_GetError());
        }

        background_music = Mix_LoadMUS("..\\art\\sounds\\166508__yoyodaman234__concrete-footstep-2.wav");
        nc_expect(background_music, "Failed to load music file! '{}'", Mix_GetError());

        Mix_PlayMusic(background_music, -1);


        return true;
    }

    //==============================================================================
    void SoundSystem::on_event(ModuleEvent& event)
    {
        switch (event.type)
        {
        case ModuleEventType::game_update:
        {
            this->update(event.update.dt);
            break;
        }

        case ModuleEventType::terminate:
        {
            this->terminate();
            break;
        }
        }
    }

    void SoundSystem::play_oneshot(const SoundResource& sound)
    {
        (void)sound;
    }

    void SoundSystem::play(const SoundResource& sound)
    {
        (void)sound;
    }

    //==============================================================================
    void SoundSystem::terminate()
    {
    }

    //==============================================================================
    void SoundSystem::update(f32 delta_seconds)
    {
        (void)delta_seconds;
    }


    void SoundHandle::play()
    {
    }

    void SoundHandle::pause()
    {
    }

    void SoundHandle::kill()
    {
    }

    void SoundHandle::set_volume(float volume01)
    {
        (void)volume01;
    }

    void SoundHandle::set_playpoint(float point01)
    {
        (void)point01;
    }

    bool SoundHandle::is_playing() const
    {
        return false;
    }

}
