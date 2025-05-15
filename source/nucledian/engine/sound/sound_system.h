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
        void play();
        void pause();
        void kill();
        void set_volume(float volume01);
        void set_playpoint(float point01);
        bool is_playing() const;

        friend class SoundSystem;
    private:
        SoundHandle(Mix_Chunk*const the_chunk, unsigned the_version)
            : chunk(the_chunk)
            , version(the_version)
        { }

        Mix_Chunk* chunk;
        unsigned version;
    };

    class SoundSystem : public IEngineModule
    {
    public:
        static EngineModuleId get_module_id();

        SoundSystem();
        bool init();

        void on_event(ModuleEvent& event) override;

        void play_oneshot(const SoundResource& sound);
        void play(const SoundResource& sound);

    private:

        void terminate();
        void update(f32 delta_seconds);


    private:
        Mix_Music *background_music = nullptr;
        std::unordered_map<Mix_Chunk*, SoundHandle> active_chunks;
    };

}