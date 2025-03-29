// Project Nucledian Source File
#pragma once

// =============== C V A R  S Y S T E M  =================== //
// 
// A system for registering configuration variables.
//
// HOW TO REGISTER CVARS
//  Put NC_REGISTER_CVAR(name, type, default value, desc) as
//  a member of the CVars structure next to the other cvars.
//  Supported types are bool, s32, f32 and std::string.
//  If you want to define a range the cvar value can be in
//  then use NC_REGISTER_CVAR_RANGED macro.
// 
// HOW TO USE CVARS FROM CODE
//  Simply access your cvar as a static member of the CVars
//  class like this: CVars::name.
//  You can obtain list of registered cvars by calling
//  CVars::get_cvar_list().
//  The values of the cvars can be changed freely from the
//  code in Release and Debug configurations. However, on
//  deploy configuartion the cvars are constexpr and can not
//  be modified.
//========================================================== //

#include <types.h>
#include <config.h>
#include <common.h>
#include <macros.h>

#include <variant>
#include <string>
#include <map>

namespace nc
{

#ifndef NC_BAKED_CVARS

#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc)    \
static inline _type _name{_default_value};                       \
static inline bool NC_TOKENJOIN(_cvar_definition, _name) = []()  \
{                                                                \
  NC_ASSERT(!get_cvar_list().contains(#_name));                  \
  get_cvar_list().insert({#_name, CVar{&_name, _desc}});         \
  return true;                                                   \
}();                                                             \

#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _desc, _range_min, _range_max) \
static inline bool NC_TOKENJOIN(_cvar_range_definition, _name) = []()                        \
{                                                                                            \
  NC_ASSERT(_range_min <= _range_max);                                                       \
  get_cvar_ranges().insert({#_name, CVarRange{_range_min, _range_max}});                     \
  return true;                                                                               \
}();                                                                                         \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

#else

#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc) \
static constexpr _type _name{_default_value};

#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _desc, _range_min, _range_max) \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

#endif


struct CVars
{
  struct CVar
  {
    using VariantPtr = std::variant<s32*, f32*, bool*, std::string*>;
    VariantPtr ptr;
    cstr       desc;
  };
  using CVarList = std::map<std::string, CVar>;

  struct CVarRange
  {
    f32 min = 0.0f;
    f32 max = 0.0f;
  };
  using CVarRanges = std::map<std::string, CVarRange>;

  static constexpr CVarRange DEFAULT_RANGE = CVarRange{-100.0f, 100.0f};

  static CVars&      get();
  static CVarList&   get_cvar_list();
  static CVarRanges& get_cvar_ranges();

  // !!! ADD YOUR CVARS HERE !!!
  NC_REGISTER_CVAR(bool, enable_top_down_debug, false, "Top down 2D debugging of the game");
  NC_REGISTER_CVAR(bool, display_debug_window,  false, "Displays debug window.");
  NC_REGISTER_CVAR(bool, display_imgui_demo,    false, "Displays imgui demo window for inspiration.");

  NC_REGISTER_CVAR_RANGED(f32, time_speed, 1.0f, "Changes the update speed.", 0.0f, 10.0f);
};

}

