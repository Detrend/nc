// Project Nucledian Source File

#pragma once

#include <config.h>

#include <cstdlib>    // std::abort

namespace nc
{

//==============================================================================
inline void NC_ERROR([[maybe_unused]]const char* msg = nullptr) noexcept
{
#ifdef NC_ASSERTS
  // TODO: log the error and dump callstack..
  std::abort();
#endif
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

#if defined(_MSC_VER) && !defined(__clang__)
#define NC_MSVC
#elif defined(_MSC_VER)
#define NC_CLANG
#endif

#define NC_TODO(_msg) __pragma(message ("TODO: " _msg))

#ifdef NC_CLANG
#define NC_FORCE_INLINE __attribute__((always_inline))
#elif defined(NC_MSVC)
#define NC_FORCE_INLINE __forceinline
#endif

