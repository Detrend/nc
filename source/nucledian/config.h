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
#define NC_PROFILING
#endif

