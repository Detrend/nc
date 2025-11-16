# project setup
cmake_minimum_required(VERSION 3.15)
project(nc LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build configurations
set(CMAKE_CONFIGURATION_TYPES
    Debug
    Release
    Deploy
    Profiling
    CACHE STRING "Build types" FORCE
)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_SOURCE_DIR}/bin/${CONFIG}")

    if(NOT CONFIG STREQUAL "Debug" AND NOT CONFIG STREQUAL "Release")
        set(CMAKE_C_FLAGS_${CONFIG_UPPER} "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "C flags for ${CONFIG}" FORCE)
        set(CMAKE_CXX_FLAGS_${CONFIG_UPPER} "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "CXX flags for ${CONFIG}" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_${CONFIG_UPPER} "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Exe linker flags for ${CONFIG}" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS_${CONFIG_UPPER} "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Shared linker flags for ${CONFIG}" FORCE)
    endif()

endforeach()

# function to get list of source files
function(get_sources IN_FOLDER OUT_VAR_NAME)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.cpp"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.h"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.hpp"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.inl"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.frag"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.vert"
        "${CMAKE_SOURCE_DIR}/${IN_FOLDER}/*.comp"
    )
    set(${OUT_VAR_NAME} ${SOURCES} PARENT_SCOPE)
endfunction()

# ===== included libraries =====

add_subdirectory(source/libs/benchmark)
add_subdirectory(source/libs/SDL2)
add_subdirectory(source/libs/SDL_mixer)

# ===== GLAD =====

add_library(glad_lib STATIC "${CMAKE_SOURCE_DIR}/source/libs/glad/glad.c")
target_include_directories(glad_lib PUBLIC "${CMAKE_SOURCE_DIR}/source/libs")

# ===== STB =====

add_library(stb_lib STATIC "${CMAKE_SOURCE_DIR}/source/libs/stb/stb.cpp")
target_include_directories(stb_lib 
    PUBLIC  "${CMAKE_SOURCE_DIR}/source/libs/stb"
    PRIVATE "${CMAKE_SOURCE_DIR}/source/nucledian"
)

# ===== IMGUI and IMPLOT =====

add_library(imgui_lib STATIC)
target_include_directories(imgui_lib PUBLIC "${CMAKE_SOURCE_DIR}/source/libs/imgui")
target_sources(imgui_lib PRIVATE
    "${CMAKE_SOURCE_DIR}/source/libs/imgui/implot.cpp"
    "${CMAKE_SOURCE_DIR}/source/libs/imgui/implot_demo.cpp"
    "${CMAKE_SOURCE_DIR}/source/libs/imgui/implot_items.cpp"
)
target_sources(imgui_lib PRIVATE
    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>:${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_demo.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_draw.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_impl_opengl3.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_impl_sdl2.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_stdlib.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_tables.cpp;${CMAKE_SOURCE_DIR}/source/libs/imgui/imgui_widgets.cpp>"
)
target_link_libraries(imgui_lib PRIVATE SDL2)

# ===== nucledian =====

get_sources("source/nucledian" NUCLEDIAN_SOURCES)
add_executable(nucledian ${NUCLEDIAN_SOURCES})
target_include_directories(nucledian PRIVATE
    "${CMAKE_SOURCE_DIR}/source/libs"
    "${CMAKE_SOURCE_DIR}/source/nucledian"
    glad_lib
    stb_lib
    imgui_lib
)
target_link_directories(nucledian PRIVATE "${CMAKE_SOURCE_DIR}/lib")
target_link_libraries(nucledian PRIVATE
    opengl32.lib
    glad_lib
    stb_lib
    imgui_lib
    "$<$<CONFIG:Profiling>:benchmark>"
    SDL2
    SDL2main
    SDL_mixer
)
target_compile_definitions(nucledian PRIVATE
    _CONSOLE
    "$<$<CONFIG:Debug>:NC_Debug;NDEBUG>"
    "$<$<CONFIG:Release>:NC_Release;NDEBUG>"
    "$<$<CONFIG:Deploy>:NC_Deploy;_DEBUG>"
    "$<$<CONFIG:Profiling>:NC_Profiling;NDEBUG>"
)

# compiler options
if(MSVC)
    target_compile_options(nucledian PRIVATE
        /W4          # WarningLevel Level4
        /sdl         # Security Development Lifecycle
        /permissive- # ConformanceMode
        /WX          # TreatWarningAsError
        /arch:AVX    # AdditionalOptions
        /fp:except-  # FloatingPointExceptions false
    )
    set_target_properties(nucledian PROPERTIES MSVC_MULTIPROCESSOR_COMPILATION TRUE)
else()
    target_compile_options(nucledian PRIVATE
        -Wall              # "Warning all"
        -Wextra            # Adds even more warnings
        -pedantic          # Enforces strict standards conformance
        -Werror            # Treat warnings as errors
        -mavx              # Enable AVX instructions
        -fno-trapping-math # Disable floating point exceptions
    )
endif()

# configuration specific compiler options
if(MSVC)
    target_compile_options(nucledian PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:/Gy>"      # FunctionLevelLinking
        "$<$<NOT:$<CONFIG:Debug>>:/Oi>"      # IntrinsicFunctions
        "$<$<NOT:$<CONFIG:Debug>>:/GL>"      # WholeProgramOptimization
        "$<$<CONFIG:Deploy>:/O2>"            # Optimization MaxSpeed
        "$<$<CONFIG:Deploy>:/GS->"           # BufferSecurityCheck false
        "$<$<CONFIG:Profiling>:/GS->"        # BufferSecurityCheck false
    )
    
    target_link_options(nucledian PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:/LTCG>"    # Link Time Code Generation
    )

else()
    target_compile_options(nucledian PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>"
        "$<$<NOT:$<CONFIG:Debug>>:-fdata-sections>"
        "$<$<NOT:$<CONFIG:Debug>>:-flto>"
        "$<$<CONFIG:Deploy>:-O2>"
        "$<$<CONFIG:Deploy>:-fno-stack-protector>"
        "$<$<CONFIG:Profiling>:-fno-stack-protector>"
    )
    
    target_link_options(nucledian PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:-flto>"
    )
endif()

set_target_properties(nucledian PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT nucledian)