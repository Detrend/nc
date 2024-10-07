// Project Nucledian Source File

#pragma once

#include <cstdlib>    // std::abort

namespace nc
{

//==============================================================================
inline void NC_ERROR([[maybe_unused]]const char* msg = nullptr) noexcept
{
  // TODO: log the error and dump callstack..
  std::abort();
}

//==============================================================================
template<typename T>
inline void NC_ASSERT(T&& check, const char* msg = nullptr) noexcept
{
  if (!(check))
  {
    NC_ERROR(msg);
  }
}

}

#define NC_TODO(_msg) __pragma(message ("TODO: " _msg))

