// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <common.h>

#include <variant>
#include <string>
#include <map>

namespace nc
{

// TODO: move these somewhere else
#define NC_TOKENPASTE2(_a, _b) _a##_b
#define NC_TOKENPASTE(_a, _b) NC_TOKENPASTE2(_a, _b)

// CVar registration macro
// Use this to register your cvars as a members of CVars struct.
// All cvars are registered as static members. Therefore, you can
// access them using CVars::your_cvar_name.
// In deploy builds the cvars are constexpr and therefore can be
// optimized out by the compiler.
// Register your cvars like this: NC_REGISTER_CVAR(type, cvar_name, default_value);
#ifndef NC_BAKED_CVARS

#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc)    \
static inline _type _name{_default_value};                       \
static inline bool NC_TOKENPASTE(_cvar_definition, _name) = []() \
{                                                                \
  NC_ASSERT(!get_cvar_list().contains(#_name));                  \
  get_cvar_list().insert({#_name, CVar{&_name, _desc}});         \
  return true;                                                   \
}();                                                             \

#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _desc, _range_min, _range_max) \
static inline bool NC_TOKENPASTE(_cvar_range_definition, _name) = []()                       \
{                                                                                            \
  NC_ASSERT(_range_min <= _range_max);                                                       \
  get_cvar_ranges().insert({#_name, CVarRange{_range_min, _range_max}});                     \
  return true; \
}();                                                                                         \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

#else
#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc) \
static constexpr _type _name{_default_value};

#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _desc, _range_min, _range_max) \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

#endif


class CVars
{
public:
  struct CVar
  {
    std::variant<s32*, f32*, bool*, std::string*> ptr;
    cstr                                          desc;
  };
  using CVarList = std::map<std::string, CVar>;

  struct CVarRange
  {
    float min = 0.0f;
    float max = 0.0f;
  };
  using CVarRanges = std::map<std::string, CVarRange>;

  static constexpr CVarRange DEFAULT_RANGE = CVarRange{-100.0f, 100.0f};

public:
  static CVars&      get();
  static CVarList&   get_cvar_list();
  static CVarRanges& get_cvar_ranges();

public: // Add your cvars here
  NC_REGISTER_CVAR(bool, enable_top_down_debug, false, "Top down 2D debugging of the game");
  NC_REGISTER_CVAR(bool, display_debug_window,  false, "Displays debug window.");
  NC_REGISTER_CVAR(bool, display_imgui_demo,    false, "Displays imgui demo window for inspiration.");

  NC_REGISTER_CVAR_RANGED(f32, time_speed,            1.0f,  "Changes the update speed.", 0.0f, 10.0f);

  NC_REGISTER_CVAR(std::string, test_string_input, "empty",  "Changes the update speed.");
};

}

