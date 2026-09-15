// Project Nuclidean Source File

#pragma once

#include <config.h>
#include <iostream>
#include <format>

#include <cstdlib>    // std::abort

#include <logging.h>

namespace nc
{

#if defined(_MSC_VER) && !defined(__clang__)
#   define NC_MSVC
#elif defined(__clang__)
#   define NC_CLANG
#endif

#if defined(_WIN32)
#   define NC_OS_WINDOWS
#elif defined(__linux__)
#   define NC_OS_LINUX
#endif



#if defined(NC_CLANG)
#   define NC_FORCE_INLINE __attribute__((always_inline))
#   define NC_NEVER_INLINE __attribute__((noinline))
#   define NC_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#   define NC_PUSH_PACKED _Pragma("pack(push, 1)")
#   define NC_POP_PACKED  _Pragma("pack(pop)")
#elif defined(NC_MSVC)
#   define NC_FORCE_INLINE __forceinline
#   define NC_NEVER_INLINE __declspec(noinline)
#   define NC_ANALYZER_NORETURN
#   define NC_PUSH_PACKED __pragma(pack(push)); __pragma(pack(1));
#   define NC_POP_PACKED  __pragma(pack(pop));
#endif

#ifdef NC_OS_WINDOWS
#   define NC_DEBUGBREAK() __debugbreak()
#   define NC_TODO(_msg) __pragma(message ("TODO: " _msg))
#elif defined(NC_OS_LINUX)
#   define NC_DEBUGBREAK()
#   define NC_TODO(_msg)
#endif


// disable warnings for when if-condition evaluates to constant
#pragma warning(disable:4127)


NC_ANALYZER_NORETURN inline void assert_fail_impl(const char* const expression_str, const logging::LoggingContext &logging_ctx, const std::string& message)
{
  std::string actual_message;
  if (expression_str)
  {
    actual_message.append(std::format("Assert: `{}`", expression_str));
  }

  if (!message.empty())
  {
    actual_message.append(std::format(" '{}'", message));
  }

  logging::log_message_impl(logging::LoggingSeverity::error, actual_message, logging_ctx);
  NC_DEBUGBREAK();
}

//==============================================================================
#   define nc_expect(expr, ...) do { if(!(expr)) nc::assert_fail_impl(STRINGIFY(expr), CAPTURE_CURRENT_LOGGING_CONTEXT(), std::format("" __VA_ARGS__));} while(false)

//==============================================================================
#if NC_ASSERTS
#   define nc_assert(expr, ...) nc_expect(expr, __VA_ARGS__)
#else
#   define nc_assert(expr, ...) do { if (false) { (void)(expr); } } while(false)
#endif


}


//==============================================================================
// Casting macros
#define cast   static_cast
#define recast reinterpret_cast
