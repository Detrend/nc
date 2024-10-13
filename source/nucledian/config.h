// Project Nucledian Source File
#pragma once

#if defined(NC_Debug) || defined(NC_Release)
#define NC_EDITOR
#endif

#if defined(NC_Debug) || defined(NC_Release) || defined(NC_Profiling)
#define NC_TESTS
#endif

#if defined(NC_Debug) || defined(NC_Release)
#define NC_ASSERTS
#endif

#if defined(NC_Debug) || defined(NC_Release)
#define NC_DEBUG_DRAW
#endif

#if defined (NC_Profiling)
#define NC_PROFILING // code profiling tools should be on
#define NC_BENCHMARK // benchmarks should be compiled

// Google benchmark library requires this in order to link.
#pragma comment (lib, "Shlwapi.lib")
#define BENCHMARK_STATIC_DEFINE
#endif

