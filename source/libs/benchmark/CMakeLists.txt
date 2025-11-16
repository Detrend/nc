add_library(benchmark STATIC)

file(GLOB BENCHMARK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
target_sources(benchmark PRIVATE ${BENCHMARK_SOURCES})

target_include_directories(benchmark
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(benchmark
    PUBLIC
        BENCHMARK_STATIC_DEFINE
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        HAVE_STD_REGEX
        HAVE_STEADY_CLOCK
)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark.cc"
    PROPERTIES COMPILE_DEFINITIONS BENCHMARK_VERSION="v1.8.5"
)

set_target_properties(benchmark PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)
if(WIN32)
    target_link_libraries(benchmark PUBLIC Shlwapi.lib)
endif()

if(MSVC)
    target_compile_options(benchmark PRIVATE
        /W4     # WarningLevel
        /EHsc   # ExceptionHandling
        
        "$<$<CONFIG:Debug>:/RTC1>" # BasicRuntimeChecks
        "$<$<CONFIG:Debug>:/Od>"   # Optimization Disabled
        
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:Profiling>>:/O2>"   # MaxSpeed
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:Profiling>>:/Ob2>"  # InlineFunctionExpansion
    )
else()
    target_compile_options(benchmark PRIVATE
        -Wall
        -Wextra
        -pedantic
        "$<$<CONFIG:Debug>:-O0>" # No optimization
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:Profiling>>:-O2>" # Max optimization
    )
endif()