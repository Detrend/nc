// Project Nucledian Source File
#pragma once

#include <utility>

#include <types.h>
#include <config.h>
#include <transform.h>
#include <math/vector.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/graphics/debug_camera.h>

#include "easings.h"

namespace nc
{
    enum class TweenSetterReturnValue {
        ok = 0,
        tween_is_invalid = 1
    };


    struct TweenBase {
        using on_finish_callback_t = std::function<void(TweenBase &tween)>;
        using ease_t = easingsf::easing_func_t;

        TweenBase(float the_duration_seconds, ease_t* the_easing_func) 
            : tween_speed(1.0f/the_duration_seconds)
            , easing_func(the_easing_func)
        {}

        virtual ~TweenBase(){}

        void stop() {
            is_done_flag = true;
        }
        bool is_done() {
            return this->is_done_flag;
        }
        float get_progress() { return t; }
        float get_value() { return easing_func(t); }

        void update(const float delta) {
            if (is_done_flag) return;
            t = min(END, t + delta * tween_speed);
            if (t >= END) {
                is_done_flag = true;
            }
            set_value(easing_func(t));
        }

        TweenBase& add_on_finished_callback(TweenBase::on_finish_callback_t&& new_callback) {
            on_finish_callbacks.emplace_back(std::move(new_callback));
            return *this;
        }



    protected:

        friend class TweenSystem;

        virtual void set_value(float t) = 0;

        static constexpr float BEGIN = 0.0f;
        static constexpr float END = 1.0f;

        bool is_done_flag = false;
        float t = 0.0f;
        float tween_speed = 1.0f;
        ease_t* easing_func;
        std::vector<on_finish_callback_t> on_finish_callbacks;
    };

    template<typename TValue, typename TSetter>
    struct Tween : public TweenBase {
    public:

        Tween(float the_duration_seconds, ease_t* the_easing_func , const TValue& the_start_value, const TValue & the_end_value, TSetter &&the_setter)
            : TweenBase(the_duration_seconds, the_easing_func)
            , start_value(the_start_value)
            , end_value(the_end_value)
            , setter(the_setter)
        { }

        void then_to(const TValue& new_end_value, float duration, TweenBase::ease_t* easing) {
            on_finish_callbacks.emplace_back([new_end_value, duration, easing]() {
                //TODO
                });
        }

        Tween<TValue, TSetter>& add_on_finished_callback(TweenBase::on_finish_callback_t&& new_callback) {
            TweenBase::add_on_finished_callback(std::move(new_callback));
            return *this;
        }

    protected:

        virtual void set_value(float factor) override {
            TValue to_set = lerp(start_value, end_value, factor);
            if (setter(to_set) != TweenSetterReturnValue::ok) {
                is_done_flag = true;
            }
        }

    private:

        TValue start_value, end_value;
        TSetter setter;


    };





    class TweenSystem;
    class TweenSystem : public IEngineModule
    {
    public:
        static EngineModuleId get_module_id();

        TweenSystem();
        bool init();

        void on_event(ModuleEvent& event) override;


        template<typename TValue, typename TSetter>
        Tween<TValue, TSetter>& tween(const TValue& the_start_value, const TValue& the_end_value, float the_duration_seconds, TweenBase::ease_t* the_easing_func, TSetter &&the_setter) {
            auto & ret = running_tweens.emplace_back(std::make_unique<Tween<TValue, TSetter>>(the_duration_seconds, the_easing_func, the_start_value, the_end_value, std::move(the_setter)));
            return *static_cast<Tween<TValue, TSetter>*>(ret.get());
        }

        auto &test() {
            static float begin = -10.0f, end = 99.9f;
            std::swap(begin, end);
            return tween(begin, end, 3.0f, &easingsf::linear, [](float value) {
                nc_log("tween: {}", value);
                return TweenSetterReturnValue::ok;
            });
        }

        auto fff() -> decltype(std::declval<TweenSystem&>().test()) {
            auto& ret = test();
            ret.add_on_finished_callback([this](const auto&) {
                fff();
            });
            return ret;
        }

    private:

        void terminate();
        void update(f32 delta_seconds);
    
        std::vector<std::unique_ptr<TweenBase>> running_tweens;
    };


}