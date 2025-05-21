// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/tweens/tween_system.h>

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
    TweenSystem::TweenSystem() { }

    //==============================================================================
    EngineModuleId TweenSystem::get_module_id()
    {
        return EngineModule::tween_system;
    }

    //==============================================================================
    bool TweenSystem::init()
    {
        test();
        return true;
    }

    //==============================================================================
    void TweenSystem::on_event(ModuleEvent& event)
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

    //==============================================================================
    void TweenSystem::terminate()
    {
        running_tweens.clear();
    }

    //==============================================================================
    void TweenSystem::update(f32 delta_seconds)
    {
        for (size_t t = 0; t < running_tweens.size(); ++t) {
            auto& tw = running_tweens[t];
            tw->update(delta_seconds);
            if (tw->is_done()) {
                for (auto&& callback : tw->on_finish_callbacks) 
                    callback();
                running_tweens.erase(running_tweens.begin() + t);
                --t;
            }
        }
    }

}
