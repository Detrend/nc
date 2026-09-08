// Project Nuclidean Source File
#pragma once

#define NC_CONFIG_ON  1
#define NC_CONFIG_OFF 0

#if defined(NC_Debug) || defined(NC_Test) || defined(NC_Profiling)
#define NC_TESTS      NC_CONFIG_ON
#define NC_EDITOR     NC_CONFIG_ON
#define NC_DEBUG_DRAW NC_CONFIG_ON
#define NC_IMGUI      NC_CONFIG_ON
#define NC_ASSERTS    NC_CONFIG_ON
#define NC_PROFILING  NC_CONFIG_ON
#define NC_HOT_RELOAD NC_CONFIG_ON
#else
#define NC_TESTS      NC_CONFIG_OFF
#define NC_EDITOR     NC_CONFIG_OFF
#define NC_DEBUG_DRAW NC_CONFIG_OFF
#define NC_IMGUI      NC_CONFIG_OFF
#define NC_ASSERTS    NC_CONFIG_OFF
#define NC_PROFILING  NC_CONFIG_OFF
#define NC_HOT_RELOAD NC_CONFIG_OFF
#endif

#if defined(NC_Profiling)
#define NC_BENCHMARK NC_CONFIG_ON
#define BENCHMARK_STATIC_DEFINE
#else
#define NC_BENCHMARK NC_CONFIG_OFF
#endif

#if defined(NC_Ship)
#define NC_BAKED_CVARS NC_CONFIG_ON
#define NC_IS_SHIP     NC_CONFIG_ON
#else
#define NC_BAKED_CVARS NC_CONFIG_OFF
#define NC_IS_SHIP     NC_CONFIG_OFF
#endif

#if defined(__clang__)
#define NC_COMPILER_CLANG NC_CONFIG_ON
#define NC_COMPILER_MSVC  NC_CONFIG_OFF
#elif defined(_MSC_VER)
#define NC_COMPILER_CLANG NC_CONFIG_OFF
#define NC_COMPILER_MSVC  NC_CONFIG_ON
#else
#error Unsupported compiler
#endif
