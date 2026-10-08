workspace "Nuclidean"
    architecture "x86_64"
    configurations { "Debug", "Test", "Profiling", "Ship" }
    newoption {
        trigger = "cppdialect",
        value = "DIALECT",
        description = "C++ language standard",
        allowed = {
            { "C++20", "C++20" },
            { "C++23", "C++23" }
        },
        default = "C++20"
    }

    -- Use either of these at generation time:
    --   .\tools\premake5.exe --cc=clang ninja
    --   .\tools\premake5.exe --cc=msc vs2026

    -- Filters that can be nested
    _filter_stack = {}
    function _set_filter_to_stack()
        local actual_filters = {}
        for _, f in ipairs(_filter_stack) do
            for _, v in ipairs(f) do
                table.insert(actual_filters, v)
            end
        end
        --print("workspace filter: " .. table.concat(actual_filters, ", "))
        filter(actual_filters)
    end
    function fbegin(args) -- push filter to the stack
        if type(args) ~= "table" then args = { args } end
        _filter_stack[#_filter_stack + 1] = args   
        _set_filter_to_stack()
    end
    function fend() -- pop filter from the stack
        table.remove(_filter_stack, #_filter_stack)
        _set_filter_to_stack()
    end 
    function felse(args) -- change filter on top of stack to different one
        fend()
        fbegin(args)
    end

    location "build"
    targetdir "bin/%{prj.name}_%{cfg.buildcfg}"
    objdir "bin_temp/%{prj.name}_%{cfg.buildcfg}"
    startproject "Nuclidean"

    externalincludedirs {
        "source/libs",
        "source/libs/SDL2/include",
        "source/libs/SDL_mixer/include"
    }

    language "C++"
    cppdialect (_OPTIONS.cppdialect or "C++20")
    warnings "Off"
    conformancemode "On"
    intrinsics "On"
    vectorextensions "AVX"
    multiprocessorcompile "On"
    dpiawareness "High"

    fbegin "configurations:Debug"
        symbols "On"
        optimize "Off"
        linktimeoptimization "Off"
        buffersecuritycheck "on"
        functionlevellinking "on"
        editandcontinue "on"
        fbegin {"system:windows"}
            runtime "Debug"
        felse {"system:not windows"}
            defines "_ITERATOR_DEBUG_LEVEL=2"
        fend()
    fend()
    fbegin "configurations:Test"
        symbols "On"
        optimize "Full"
        linktimeoptimization "On"
        buffersecuritycheck "on"
        functionlevellinking "on"
        editandcontinue "on"
        fbegin {"system:windows"}
            runtime "Release"
        felse {"system:not windows"}
           defines "_ITERATOR_DEBUG_LEVEL=0"
        fend()
    fend()
    fbegin "configurations:Profiling"
        symbols "On"
        optimize "Full"
        linktimeoptimization "On"
        buffersecuritycheck "on"
        functionlevellinking "on"
        editandcontinue "on"
        fbegin "system:windows"
            runtime "Release"
        felse "system:not windows"
           defines "_ITERATOR_DEBUG_LEVEL=0"
        fend()
    fend()
    fbegin "configurations:Ship"
        symbols "Off"
        optimize "Full"
        linktimeoptimization "On"
        buffersecuritycheck "off"
        functionlevellinking "on"
        editandcontinue "off"
        fbegin {"system:windows"}
            runtime "Release"
        felse {"system:not windows"}
           defines "_ITERATOR_DEBUG_LEVEL=0"
        fend()
    fend()

project "Nuclidean"
    files {
        "source/nuclidean/**.h",
        "source/nuclidean/**.inl",
        "source/nuclidean/**.cpp",
        "resource/*"
    }
    includedirs "source/nuclidean"
    uses { "glad", "glm", "stb", "SDL_mixer" }
    fbegin "system:windows"
        uses { "SDL2" }
    felse "system:not windows"
        linkoptions { "-lSDL2" }
        links "wayland-client"
    fend()
    defines { "_CONSOLE", "SDL_MAIN_HANDLED" }

    warnings "Extra"
    fatalwarnings "All"
    fbegin "toolset:clang"
        enablewarnings {
            -- Control flow
            "comma", "conditional-uninitialized", "implicit-fallthrough",
            "missing-noreturn", "unreachable-code-aggressive",

            -- Conversions
            "anon-enum-enum-conversion", "bitfield-enum-conversion",
            "enum-conversion", "float-overflow-conversion", "shorten-64-to-32",
            "string-conversion",

            -- Casts and comparisons
            "cast-qual", "old-style-cast", "shift-sign-overflow",
            "tautological-constant-in-range-compare", "undefined-reinterpret-cast",

            -- Memory layout
            "array-bounds-pointer-arithmetic", "class-varargs", "over-aligned", "unaligned-access",

            -- Classes
            "deprecated-copy-with-dtor", "duplicate-enum", "non-virtual-dtor",
            "range-loop-bind-reference", "shadow-field-in-constructor-modified",
            "suggest-destructor-override", "suggest-override",

            -- Declarations and unused code
            "missing-prototypes", "missing-variable-declarations",
            "unused-macros", "unused-member-function", "unused-template",

            -- Source hygiene
            "header-hygiene", "invalid-utf8", "newline-eof", "undef"
        }
        disablewarnings { 
            "switch",
            "#pragma-messages",
            "missing-field-initializers",
            "missing-designated-field-initializers"
        }
        fbegin "system:not windows"
            -- additional warnings disabled for now so that we can compile on Linux. TODO: fix the code to not throw these warnings
            disablewarnings { 
                "unknown-pragmas",
                "unaligned-access",
                "unused-command-line-argument"
            }
        fend()
    fend()
    characterset "Unicode"
    clr "Off"
    resincludedirs "resource"

    fbegin "configurations:Debug"
        kind "ConsoleApp"
        defines "NC_Debug"
        uses "imgui"
        fbegin "toolset:clang"
            buildoptions {
                "-Wno-error=unused",
                "-Wno-error=unused-parameter",
                "-Wno-error=unused-macros",
                "-Wno-error=unused-member-function",
                "-Wno-error=unused-template",
                "-Wno-error=unreachable-code-aggressive"
            }
        fend()
    felse "configurations:Test"
        kind "ConsoleApp"
        defines { "NC_Test", "NDEBUG" }
        uses "imgui"
    felse "configurations:Profiling"
        kind "ConsoleApp"
        defines { "NC_Profiling", "NDEBUG" }
        uses { "imgui", "benchmark" }
    felse "configurations:Ship"
        kind "WindowedApp"
        defines { "NC_Ship", "NDEBUG" }
        fbegin "system:windows"
            entrypoint "mainCRTStartup"
            fbegin  "action:ninja" 
                -- workaround for ninja ignoring `kind "WindowedApp"`
                linkoptions { "-Xlinker /SUBSYSTEM:WINDOWS", "-Xlinker /ENTRY:mainCRTStartup" }
            fend()
        fend()
    felse {"action:ninja", "system:windows"}
        linkoptions { "-fuse-ld=lld" }
    fend()

-- ############################ 3rd party libraries ############################

project "benchmark"
    kind "StaticLib"
    characterset "MBCS"

    files { "source/libs/benchmark/**.h", "source/libs/benchmark/**.cc" }
    removefiles "source/libs/benchmark/src/benchmark_main.cc"
    includedirs { "source/libs/benchmark/include", "source/libs/benchmark/src" }
    defines {
        "BENCHMARK_STATIC_DEFINE",
        "HAVE_STD_REGEX",
        "HAVE_STEADY_CLOCK",
        'BENCHMARK_VERSION="v1.8.5"'
    }
    fbegin "system:windows"
        defines { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }
    fend()

    usage "PUBLIC"
        includedirs "source/libs/benchmark/include"
    usage "INTERFACE"
        links { "benchmark" }
    fbegin "system:windows"
        links { "shlwapi" }
    fend()

    fbegin "configurations:not Debug"
        defines "NDEBUG"
    felse "configurations:not Profiling"
        excludefrombuild "On"
    fend()

project "glad"
    kind "StaticLib"
    files "source/libs/glad/*"
    includedirs "source/libs/glad"
    usage "INTERFACE"
        links { "glad" }

project "glm"
    kind "StaticLib"
    files {
        "source/libs/glm/**.h",
        "source/libs/glm/**.hpp",
        "source/libs/glm/**.inl",
        "source/libs/glm/**.cpp",
    }
    removefiles "source/libs/glm/**.cppm"
    includedirs "source/libs/glm"
    usage "INTERFACE"
        links "glm"

project "imgui"
    kind "StaticLib"
    files "source/libs/imgui/*"
    removefiles "source/libs/imgui/imgui_impl_glfw.*"
    includedirs "source/libs/imgui"
    usage "INTERFACE"
        links "imgui"

project "stb"
    kind "StaticLib"
    files "source/libs/stb/*"
    includedirs "source/libs/stb"
    usage "INTERFACE"
        links "stb"

if os.target() == "windows" then
    project "SDL2"
        kind "StaticLib"

        files {
            "source/libs/SDL2/**.h",
            "source/libs/SDL2/src/*.c",
            "source/libs/SDL2/src/atomic/*.c",
            "source/libs/SDL2/src/audio/*.c",
            "source/libs/SDL2/src/cpuinfo/*.c",
            "source/libs/SDL2/src/dynapi/*.c",
            "source/libs/SDL2/src/events/*.c",
            "source/libs/SDL2/src/file/*.c",
            "source/libs/SDL2/src/haptic/*.c",
            "source/libs/SDL2/src/hidapi/*.c",
            "source/libs/SDL2/src/joystick/*.c",
            "source/libs/SDL2/src/libm/*.c",
            "source/libs/SDL2/src/locale/*.c",
            "source/libs/SDL2/src/misc/*.c",
            "source/libs/SDL2/src/power/*.c",
            "source/libs/SDL2/src/render/*.c",
            "source/libs/SDL2/src/sensor/*.c",
            "source/libs/SDL2/src/stdlib/*.c",
            "source/libs/SDL2/src/thread/*.c",
            "source/libs/SDL2/src/timer/*.c",
            "source/libs/SDL2/src/video/*.c",
            "source/libs/SDL2/src/*/windows/*.c",
            "source/libs/SDL2/src/audio/directsound/*.c",
            "source/libs/SDL2/src/audio/disk/*.c",
            "source/libs/SDL2/src/audio/dummy/*.c",
            "source/libs/SDL2/src/audio/wasapi/*.c",
            "source/libs/SDL2/src/audio/winmm/*.c",
            "source/libs/SDL2/src/haptic/dummy/*.c",
            "source/libs/SDL2/src/joystick/dummy/*.c",
            "source/libs/SDL2/src/joystick/hidapi/*.c",
            "source/libs/SDL2/src/joystick/virtual/*.c",
            "source/libs/SDL2/src/render/direct3d/*.c",
            "source/libs/SDL2/src/render/direct3d11/*.c",
            "source/libs/SDL2/src/render/direct3d12/*.c",
            "source/libs/SDL2/src/render/opengl/*.c",
            "source/libs/SDL2/src/render/opengles2/*.c",
            "source/libs/SDL2/src/render/software/*.c",
            "source/libs/SDL2/src/sensor/dummy/*.c",
            "source/libs/SDL2/src/thread/generic/SDL_syscond.c",
            "source/libs/SDL2/src/video/dummy/*.c",
            "source/libs/SDL2/src/video/yuv2rgb/*.c",
        }
        removefiles {
            "source/libs/SDL2/src/hidapi/windows/**",
            "source/libs/SDL2/src/events/imKStoUCS.c",
            "source/libs/SDL2/src/events/SDL_keysym_to_scancode.c",
            "source/libs/SDL2/src/events/SDL_scancode_tables.c",
            "source/libs/SDL2/src/main/windows/SDL_windows_main.c",
        }
        includedirs {
            "source/libs/SDL2/include",
            "source/libs/SDL2/src/video/khronos"
        }
        defines "_WINDOWS"

        usage "INTERFACE"
            links {
                "SDL2", 
                "user32", 
                "gdi32", 
                "winmm", 
                "imm32", 
                "ole32", 
                "oleaut32", 
                "version", 
                "uuid", 
                "advapi32", 
                "setupapi", 
                "shell32" 
            }

        fbegin "configurations:Debug"
            defines "_DEBUG"
        felse "configurations:not Debug"
            defines "NDEBUG"
        fend()
end

project "SDL_mixer"
    kind "StaticLib"

    files {
        "source/libs/SDL_mixer/**.h",
        "source/libs/SDL_mixer/src/*.c",
        "source/libs/SDL_mixer/src/codecs/*.c",
    }
    includedirs {
        "source/libs/SDL_mixer/include",
        "source/libs/SDL_mixer/src",
        "source/libs/SDL_mixer/src/codecs",
        "source/libs/SDL_mixer/src/codecs/timidity",
        "source/libs/SDL_mixer/src/codecs/native_midi",
    }
    links { "SDL2" }
    defines {"MUSIC_WAV", "MUSIC_MP3_MINIMP3" }

    fbegin "system:windows"
        links { "SDL2" }
        defines { "WIN32", "_WINDOWS" }
    fend()

    usage "INTERFACE"
        links "SDL_mixer"

    fbegin "configurations:Debug"
        defines "_DEBUG"
    felse "configurations:not Debug"
        defines { "NDEBUG", "_CRT_SECURE_NO_WARNINGS" }
    fend()
