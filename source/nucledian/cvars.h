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
#include <metaprogramming.h>

#include <variant>
#include <string>
#include <map>

namespace nc
{

#ifndef NC_BAKED_CVARS

// Defined as empty if cvars are not constexpr
#define NC_CVARS_CONSTEXPR

// Default cvar registration inside some struct/class
#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc)               \
static inline _type _name{_default_value};                                  \
static inline bool NC_TOKENJOIN(_cvar_definition, _name) = []()             \
{                                                                           \
  nc_assert(!::nc::CVars::get_cvar_list().contains(#_name));                \
  ::nc::CVars::get_cvar_list().insert({#_name, ::nc::CVar{&_name, _desc}}); \
  return true;                                                              \
}();

// External cvar registration, has to be done inside of a .cpp
#define NC_REGISTER_CVAR_EXTERNAL_CPP(_name, _desc) \
NC_REGISTER_CVAR_EXTERNAL_CPP_IMPL(_name, _desc, __LINE__)

// Ranged cvar registration, same as normal cvar registration but also adds a min/max range
#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _range_min, _range_max, _desc) \
static inline bool NC_TOKENJOIN(_cvar_range_definition, _name) = []()                        \
{                                                                                            \
  nc_assert(_range_min <= _range_max);                                                       \
  ::nc::CVars::get_cvar_ranges().insert({#_name, ::nc::CVarRange{_range_min, _range_max}});  \
  return true;                                                                               \
}();                                                                                         \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

// Ranged external cvar registration
#define NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED(_name, _range_min, _range_max, _desc) \
NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED(_name, _range_min, _range_max, _desc, __LINE__)

#define NC_REGISTER_CVAR_EXTERNAL_CPP_IMPL(_name, _desc, _line)             \
static inline bool NC_TOKENJOIN(_cvar_definition, _line) = []()          \
{                                                                           \
  nc_assert(!::nc::CVars::get_cvar_list().contains(#_name));                \
  ::nc::CVars::get_cvar_list().insert({#_name, ::nc::CVar{&_name, _desc}}); \
  return true;                                                              \
}();

#define NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED_IMPL(_name, _range_min, _range_max, _desc, _line) \
static inline bool NC_TOKENJOIN(_cvar_range_definition, _line) = []()                          \
{                                                                                              \
  nc_assert(_range_min <= _range_max);                                                         \
  ::nc::CVars::get_cvar_ranges().insert({#_name, ::nc::CVarRange{_range_min, _range_max}});    \
  return true;                                                                                 \
}();                                                                                           \
NC_REGISTER_CVAR_EXTERNAL_CPP_IMPL(_name, _desc, _line)

#else

#define NC_CVARS_CONSTEXPR constexpr

#define NC_REGISTER_CVAR(_type, _name, _default_value, _desc) \
static constexpr _type _name{_default_value};

#define NC_REGISTER_CVAR_EXTERNAL_CPP(...)
#define NC_REGISTER_CVAR_EXTERNAL_CPP_IMPL(...)

#define NC_REGISTER_CVAR_RANGED(_type, _name, _default_value, _range_min, _range_max, _desc) \
NC_REGISTER_CVAR(_type, _name, _default_value, _desc)

NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED(...)
NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED_IMPL(...)

#endif

struct CVar
{
  using VariantPtr = std::variant<s32*, f32*, bool*, std::string*>;
  VariantPtr ptr;
  cstr       desc;
  cstr       category;
};
using CVarList = std::map<std::string, CVar>;

struct CVarRange
{
  f32 min = 0.0f;
  f32 max = 0.0f;
};
using CVarRanges = std::map<std::string, CVarRange>;

struct CVars
{
  static constexpr CVarRange DEFAULT_RANGE = CVarRange{-100.0f, 100.0f};

  static CVars&      get();
  static CVarList&   get_cvar_list();
  static CVarRanges& get_cvar_ranges();

  // !!! ADD YOUR CVARS HERE !!!
  NC_REGISTER_CVAR(bool, enable_top_down_debug, false, "Top down 2D debugging of the game");
  NC_REGISTER_CVAR(bool, display_debug_window,  false, "Displays debug window.");
  NC_REGISTER_CVAR(bool, display_imgui_demo,    false, "Displays imgui demo window for inspiration.");
  NC_REGISTER_CVAR(bool, debug_player_raycasts, false, "Debug raycast from player's eyes.");
  NC_REGISTER_CVAR(bool, lock_camera_pitch,     false, "Restricts camera from looking up/down.");
  NC_REGISTER_CVAR(bool, has_fps_limit,         false, "Is the FPS limited?");
  NC_REGISTER_CVAR(bool, has_min_fps,           false, "Min FPS");

  NC_REGISTER_CVAR_RANGED(f32, fps_limit, 60.0f, 1.0f, 512.0f,
    "FPS limit if the \"has_fps_limit\" is turned on");
  NC_REGISTER_CVAR_RANGED(f32, fps_min, 30.0f, 1.0f, 2048.0f,
    "FPS limit if the \"has_fps_limit\" is turned on");
  NC_REGISTER_CVAR_RANGED(s32, opengl_debug_severity, 1, 0, 3,
    "0 = everything, 1 = low and higher, 2 = medium and higher, 3 = critical only");
  NC_REGISTER_CVAR_RANGED(f32, time_speed, 1.0f, 0.0f, 10.0f, "Changes the update speed.");

  NC_REGISTER_CVAR_RANGED(f32, gun_sway_amount,           0.05f, 0.0f, 1.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, gun_sway_speed,            5.5f,  0.0f, 8.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, gun_change_time,           0.4f,  0.0f, 5.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, gun_sway_move_fadein_time, 0.2f,  0.0f, 5.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, gun_sway_air_time,         0.6f,  0.0f, 3.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, gun_zoom,                  1.2f,  0.5f, 3.0f, "");

  NC_REGISTER_CVAR_RANGED(f32, camera_spring_height,       0.25f, 0.0f, 1.0f, "");
  NC_REGISTER_CVAR_RANGED(f32, camera_spring_update_speed, 3.0f, 0.1f, 10.0f, "");
};

}

