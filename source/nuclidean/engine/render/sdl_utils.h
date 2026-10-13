// Project Nuclidean Source File
#pragma once

#include <logging.h>

#include <SDL_error.h>

#define sdl_crit(call_name) (nc_crit("[render][sdl] {} failed: {}", call_name, SDL_GetError()), false)
#define sdl_warn(call_name) (nc_warn("[render][sdl] {} failed: {}", call_name, SDL_GetError()), false)
