// Project Nuclidean Source File
#pragma once

#define NC_CONFIG_TRUE  1
#define NC_CONFIG_FALSE 0
#define NC_CONFIG_ON  NC_CONFIG_TRUE
#define NC_CONFIG_OFF NC_CONFIG_FALSE

#if defined(NC_Debug) || defined(NC_Test) || defined(NC_Profiling)
#define NC_TESTS      NC_CONFIG_ON
#define NC_EDITOR     NC_CONFIG_ON
#define NC_DEBUG_DRAW NC_CONFIG_ON
#define NC_IMGUI      NC_CONFIG_ON
#define NC_ASSERTS    NC_CONFIG_ON
#define NC_PROFILING  NC_CONFIG_OFF // MR says: Turned off for now due to race conditions from render thread
#define NC_HOT_RELOAD NC_CONFIG_ON
#define NC_SCRATCH_ALLOCATOR_CHECKS NC_CONFIG_ON
#else
#define NC_TESTS      NC_CONFIG_OFF
#define NC_EDITOR     NC_CONFIG_OFF
#define NC_DEBUG_DRAW NC_CONFIG_OFF
#define NC_IMGUI      NC_CONFIG_OFF
#define NC_ASSERTS    NC_CONFIG_OFF
#define NC_PROFILING  NC_CONFIG_OFF
#define NC_HOT_RELOAD NC_CONFIG_OFF
#define NC_SCRATCH_ALLOCATOR_CHECKS NC_CONFIG_OFF
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
#define NC_COMPILER_GCC   NC_CONFIG_OFF
#elif defined(_MSC_VER)
#define NC_COMPILER_CLANG NC_CONFIG_OFF
#define NC_COMPILER_MSVC  NC_CONFIG_ON
#define NC_COMPILER_GCC   NC_CONFIG_OFF
#elif defined(__GNUC__)
#define NC_COMPILER_CLANG NC_CONFIG_OFF
#define NC_COMPILER_MSVC  NC_CONFIG_OFF
#define NC_COMPILER_GCC   NC_CONFIG_ON
#else
#error Unsupported compiler
#endif

// Render thread
#define NC_USING_RENDER_THREAD NC_CONFIG_ON

// Temporary for parts of the code that don't work with the render thread
#if NC_USING_RENDER_THREAD
  #define NC_ENABLE_ONLY_WITHOUT_RENDER_THREAD NC_CONFIG_OFF
#else
  #define NC_ENABLE_ONLY_WITHOUT_RENDER_THREAD NC_CONFIG_ON
#endif
