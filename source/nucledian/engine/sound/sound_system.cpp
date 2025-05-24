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

    Mix_Chunk* SoundSystem::get_loaded_chunk(const SoundResource& resource)
    {
        auto it = loaded_chunks.find(resource);
        if (it == loaded_chunks.end()) {
            Mix_Chunk*const ret = Mix_LoadWAV(resource);
            nc_assert(ret);
            loaded_chunks[resource] = ret;
            return ret;
        }
        return it->second;
    }

    void SoundSystem::play_oneshot(const SoundResource& sound)
    {
        play(sound);
    }

    SoundHandle SoundSystem::play(const SoundResource& sound, const bool should_loop /* = false*/)
    {
        Mix_Chunk *const chunk = get_loaded_chunk(sound);
        nc_assert(chunk);
        const int channel = Mix_PlayChannel(-1, chunk, should_loop ? -1 : 0);
        nc_assert(channel != -1);
        return SoundHandle(chunk, channel, generate_next_version_id());
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


    void SoundHandle::set_paused(const bool should_be_paused)
    {
        nc_assert(check_is_valid());
        if(should_be_paused)
            Mix_Pause(this->channel);
        Mix_Resume(this->channel);
    }

    void SoundHandle::kill()
    {
        nc_assert(check_is_valid());
        Mix_HaltChannel(this->channel);
        
    }

    static int volume_from01(float volume01) {
        return static_cast<int>(clamp(volume01, 0.0f, 1.0f) * MIX_MAX_VOLUME); 
    }
    static float volume_to01(int mixer_volume) {
        return clamp(static_cast<float>(mixer_volume) / MIX_MAX_VOLUME, 0.0f, 1.0f);
    }

    void SoundHandle::set_volume(float volume01)
    {
        nc_assert(check_is_valid());
        const int ret = Mix_VolumeChunk(this->chunk, volume_from01(volume01));
        (void)ret;
        nc_assert(ret >= 0);
    }


    bool SoundHandle::is_paused() const
    {
        nc_assert(check_is_valid());
        const int ret = Mix_Paused(this->channel);
        if (ret == 1) return true;
        if (ret == 0) return false;
        nc_assert(false, "invalid paused value '{}'", ret);
        return false;
    }

    bool SoundHandle::is_playing() const
    {
        nc_assert(check_is_valid());
        return false;
    }

    float SoundHandle::get_volume()
    {
        nc_assert(check_is_valid());
        const int ret = Mix_VolumeChunk(this->chunk, -1);
        nc_assert(ret >= 0);
        return volume_to01(ret);
    }

    TweenBase& SoundHandle::do_volume(float end_volume01, float duration_seconds, easingsf::easing_func_t ease) 
    {
        SoundSystem& sys = get_engine().get_module<SoundSystem>();

        return get_engine().get_module<TweenSystem>().tween(get_volume(), end_volume01, duration_seconds, ease, [this, &sys](float value) {
            if (!this->is_playing())
                return TweenSetterReturnValue::tween_is_invalid;;
            if (sys.active_chunks.find(this->channel)->second.version != this->version)
                return TweenSetterReturnValue::tween_is_invalid;
            this->set_volume(value);
            return TweenSetterReturnValue::ok;
            });
    }

    bool SoundHandle::check_is_valid() const {
        if (!this->chunk) return false;
        if (this->channel < 0) return false;

        auto& sound_system = get_engine().get_module<SoundSystem>();
        auto it = sound_system.active_chunks.find(channel);
        return it != sound_system.active_chunks.end() && it->second.version == this->version;
    }

}
