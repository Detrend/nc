// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <transform.h>
#include <math/vector.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/graphics/debug_camera.h>

#include "engine/tweens/tween_system.h"
#include "engine/tweens/easings.h"
#include "engine/core/engine.h"

#include <vector>
#include <unordered_set>
#include <tuple>

struct Mix_Music;
struct Mix_Chunk;

namespace nc
{
    using SoundResource = cstr;


    struct SoundHandle {

    public:
        void set_paused(const bool should_be_paused);
        void kill();
        void set_volume(float volume01);

        bool is_paused() const;
        bool is_playing() const;
        float get_volume();

        TweenBase& do_volume(float end_volume01, float duration_seconds, easingsf::easing_func_t ease = easingsf::linear);

        friend class SoundSystem;
    private:
        using channel_t = int;
        SoundHandle(Mix_Chunk* const the_chunk, channel_t the_channel, u64 the_version)
            : chunk(the_chunk)
            , channel(the_channel)
            , version(the_version)
        {
        }

        Mix_Chunk* chunk;
        channel_t channel;
        u64 version;

        bool check_is_valid(void) const;
    };


    class SoundSystem : public IEngineModule
    {
    public:
        static EngineModuleId get_module_id();

        SoundSystem();
        bool init();

        void on_event(ModuleEvent& event) override;


    private:

        void terminate();
        void update(f32 delta_seconds);


    private:
        using channel_t = SoundHandle::channel_t;

        Mix_Music *background_music = nullptr;
        std::unordered_map<channel_t, SoundHandle> active_chunks;
        std::unordered_map<SoundResource, Mix_Chunk*> loaded_chunks;
        u64 version_counter;

        friend struct SoundHandle;

        u64 generate_next_version_id() { return ++version_counter; }
        Mix_Chunk* get_loaded_chunk(const SoundResource& rsrc);

    public:

        void play_oneshot(const SoundResource& sound);
        SoundHandle play(const SoundResource& sound, const bool should_loop = false);
    };


}