// Project Nuclidean Source File

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // allows us to use std::mbstowcs, would deserve a less retarded way to do it
#endif

#include <engine/core/os.h>
#include <common.h>
#include <types.h>
#include <algorithm> // std::min
#include <cstring>   // std::strlen

#if NC_OS_WINDOWS

// Avoid windows trash
#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

// Avoid defining "min" and "max" macros
#ifndef NOMINMAX
  #define NOMINMAX
#endif

#include <Windows.h>
#include <cstdlib> // std::mbstowcs
#endif

namespace nc::os
{

//======================================================================================================================
void set_current_thread_name(cstr name)
{
#if NC_OS_WINDOWS
  constexpr u64 MAX_NAME_LEN = 64;

  // Leave space for the null terminator, the buffer is zero initialized
  wchar_t wide_string_repr[MAX_NAME_LEN]{};
  std::mbstowcs(wide_string_repr, name, std::min(MAX_NAME_LEN - 1, std::strlen(name)));

  SetThreadDescription(GetCurrentThread(), wide_string_repr);
#elif NC_OS_LINUX
  NC_TODO("Implement 'set_current_thread_name' for linux as well.");
#else
  #error "Not implemented for this OS"
#endif
}

}
