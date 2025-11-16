set(SDL2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(SDL2 SHARED)

target_sources(SDL2 PRIVATE
    "${SDL2_SOURCE_DIR}/src/SDL.c"
    "${SDL2_SOURCE_DIR}/src/SDL_assert.c"
    "${SDL2_SOURCE_DIR}/src/SDL_dataqueue.c"
    "${SDL2_SOURCE_DIR}/src/SDL_error.c"
    "${SDL2_SOURCE_DIR}/src/SDL_guid.c"
    "${SDL2_SOURCE_DIR}/src/SDL_hints.c"
    "${SDL2_SOURCE_DIR}/src/SDL_list.c"
    "${SDL2_SOURCE_DIR}/src/SDL_log.c"
    "${SDL2_SOURCE_DIR}/src/SDL_utils.c"
    "${SDL2_SOURCE_DIR}/src/atomic/SDL_atomic.c"
    "${SDL2_SOURCE_DIR}/src/atomic/SDL_spinlock.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_audio.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_audiocvt.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_audiodev.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_audiotypecvt.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_mixer.c"
    "${SDL2_SOURCE_DIR}/src/audio/SDL_wave.c"
    "${SDL2_SOURCE_DIR}/src/audio/directsound/SDL_directsound.c"
    "${SDL2_SOURCE_DIR}/src/audio/disk/SDL_diskaudio.c"
    "${SDL2_SOURCE_DIR}/src/audio/dummy/SDL_dummyaudio.c"
    "${SDL2_SOURCE_DIR}/src/audio/wasapi/SDL_wasapi.c"
    "${SDL2_SOURCE_DIR}/src/audio/wasapi/SDL_wasapi_win32.c"
    "${SDL2_SOURCE_DIR}/src/audio/winmm/SDL_winmm.c"
    "${SDL2_SOURCE_DIR}/src/core/windows/SDL_hid.c"
    "${SDL2_SOURCE_DIR}/src/core/windows/SDL_immdevice.c"
    "${SDL2_SOURCE_DIR}/src/core/windows/SDL_windows.c"
    "${SDL2_SOURCE_DIR}/src/core/windows/SDL_xinput.c"
    "${SDL2_SOURCE_DIR}/src/cpuinfo/SDL_cpuinfo.c"
    "${SDL2_SOURCE_DIR}/src/dynapi/SDL_dynapi.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_clipboardevents.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_displayevents.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_dropevents.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_events.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_gesture.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_keyboard.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_mouse.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_quit.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_touch.c"
    "${SDL2_SOURCE_DIR}/src/events/SDL_windowevents.c"
    "${SDL2_SOURCE_DIR}/src/file/SDL_rwops.c"
    "${SDL2_SOURCE_DIR}/src/filesystem/windows/SDL_sysfilesystem.c"
    "${SDL2_SOURCE_DIR}/src/haptic/SDL_haptic.c"
    "${SDL2_SOURCE_DIR}/src/haptic/dummy/SDL_syshaptic.c"
    "${SDL2_SOURCE_DIR}/src/haptic/windows/SDL_dinputhaptic.c"
    "${SDL2_SOURCE_DIR}/src/haptic/windows/SDL_windowshaptic.c"
    "${SDL2_SOURCE_DIR}/src/haptic/windows/SDL_xinputhaptic.c"
    "${SDL2_SOURCE_DIR}/src/hidapi/SDL_hidapi.c"
    "${SDL2_SOURCE_DIR}/src/joystick/SDL_gamecontroller.c"
    "${SDL2_SOURCE_DIR}/src/joystick/SDL_joystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/SDL_steam_virtual_gamepad.c"
    "${SDL2_SOURCE_DIR}/src/joystick/controller_type.c"
    "${SDL2_SOURCE_DIR}/src/joystick/dummy/SDL_sysjoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_combined.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_gamecube.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_luna.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_ps3.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_ps4.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_ps5.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_rumble.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_shield.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_stadia.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_steam.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_steamdeck.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_switch.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_wii.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_xbox360.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_xbox360w.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_xboxone.c"
    "${SDL2_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapijoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/virtual/SDL_virtualjoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/windows/SDL_dinputjoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/windows/SDL_rawinputjoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/windows/SDL_windows_gaming_input.c"
    "${SDL2_SOURCE_DIR}/src/joystick/windows/SDL_windowsjoystick.c"
    "${SDL2_SOURCE_DIR}/src/joystick/windows/SDL_xinputjoystick.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_atan2.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_exp.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_fmod.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_log.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_log10.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_pow.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_rem_pio2.c"
    "${SDL2_SOURCE_DIR}/src/libm/e_sqrt.c"
    "${SDL2_SOURCE_DIR}/src/libm/k_cos.c"
    "${SDL2_SOURCE_DIR}/src/libm/k_rem_pio2.c"
    "${SDL2_SOURCE_DIR}/src/libm/k_sin.c"
    "${SDL2_SOURCE_DIR}/src/libm/k_tan.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_atan.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_copysign.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_cos.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_fabs.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_floor.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_scalbn.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_sin.c"
    "${SDL2_SOURCE_DIR}/src/libm/s_tan.c"
    "${SDL2_SOURCE_DIR}/src/loadso/windows/SDL_sysloadso.c"
    "${SDL2_SOURCE_DIR}/src/locale/SDL_locale.c"
    "${SDL2_SOURCE_DIR}/src/locale/windows/SDL_syslocale.c"
    "${SDL2_SOURCE_DIR}/src/misc/SDL_url.c"
    "${SDL2_SOURCE_DIR}/src/misc/windows/SDL_sysurl.c"
    "${SDL2_SOURCE_DIR}/src/power/SDL_power.c"
    "${SDL2_SOURCE_DIR}/src/power/windows/SDL_syspower.c"
    "${SDL2_SOURCE_DIR}/src/render/SDL_d3dmath.c"
    "${SDL2_SOURCE_DIR}/src/render/SDL_render.c"
    "${SDL2_SOURCE_DIR}/src/render/SDL_yuv_sw.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d/SDL_render_d3d.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d/SDL_shaders_d3d.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d11/SDL_render_d3d11.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d11/SDL_shaders_d3d11.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d12/SDL_render_d3d12.c"
    "${SDL2_SOURCE_DIR}/src/render/direct3d12/SDL_shaders_d3d12.c"
    "${SDL2_SOURCE_DIR}/src/render/opengl/SDL_render_gl.c"
    "${SDL2_SOURCE_DIR}/src/render/opengl/SDL_shaders_gl.c"
    "${SDL2_SOURCE_DIR}/src/render/opengles2/SDL_render_gles2.c"
    "${SDL2_SOURCE_DIR}/src/render/opengles2/SDL_shaders_gles2.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_blendfillrect.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_blendline.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_blendpoint.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_drawline.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_drawpoint.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_render_sw.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_rotate.c"
    "${SDL2_SOURCE_DIR}/src/render/software/SDL_triangle.c"
    "${SDL2_SOURCE_DIR}/src/sensor/SDL_sensor.c"
    "${SDL2_SOURCE_DIR}/src/sensor/dummy/SDL_dummysensor.c"
    "${SDL2_SOURCE_DIR}/src/sensor/windows/SDL_windowssensor.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_crc16.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_crc32.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_getenv.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_iconv.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_malloc.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_mslibc.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_qsort.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_stdlib.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_string.c"
    "${SDL2_SOURCE_DIR}/src/stdlib/SDL_strtokr.c"
    "${SDL2_SOURCE_DIR}/src/thread/SDL_thread.c"
    "${SDL2_SOURCE_DIR}/src/thread/generic/SDL_syscond.c"
    "${SDL2_SOURCE_DIR}/src/thread/windows/SDL_syscond_cv.c"
    "${SDL2_SOURCE_DIR}/src/thread/windows/SDL_sysmutex.c"
    "${SDL2_SOURCE_DIR}/src/thread/windows/SDL_syssem.c"
    "${SDL2_SOURCE_DIR}/src/thread/windows/SDL_systhread.c"
    "${SDL2_SOURCE_DIR}/src/thread/windows/SDL_systls.c"
    "${SDL2_SOURCE_DIR}/src/timer/SDL_timer.c"
    "${SDL2_SOURCE_DIR}/src/timer/windows/SDL_systimer.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_0.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_1.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_A.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_auto.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_copy.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_N.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_blit_slow.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_bmp.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_clipboard.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_egl.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_fillrect.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_pixels.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_rect.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_RLEaccel.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_shape.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_stretch.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_surface.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_video.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_vulkan_utils.c"
    "${SDL2_SOURCE_DIR}/src/video/SDL_yuv.c"
    "${SDL2_SOURCE_DIR}/src/video/dummy/SDL_nullevents.c"
    "${SDL2_SOURCE_DIR}/src/video/dummy/SDL_nullframebuffer.c"
    "${SDL2_SOURCE_DIR}/src/video/dummy/SDL_nullvideo.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsclipboard.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsevents.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsframebuffer.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowskeyboard.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsmessagebox.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsmodes.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsmouse.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsopengl.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsopengles.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsshape.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsvideo.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowsvulkan.c"
    "${SDL2_SOURCE_DIR}/src/video/windows/SDL_windowswindow.c"
    "${SDL2_SOURCE_DIR}/src/video/yuv2rgb/yuv_rgb_lsx.c"
    "${SDL2_SOURCE_DIR}/src/video/yuv2rgb/yuv_rgb_sse.c"
    "${SDL2_SOURCE_DIR}/src/video/yuv2rgb/yuv_rgb_std.c"
)

target_include_directories(SDL2 PUBLIC "${SDL2_SOURCE_DIR}/include")

target_compile_definitions(SDL2 PRIVATE
    DLL_EXPORT
    _WINDOWS
    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<NOT:$<CONFIG:Debug>>:NDEBUG>"
)

target_link_libraries(SDL2 PRIVATE
    setupapi.lib
    winmm.lib
    imm32.lib
    version.lib

    "$<$<CONFIG:Debug>:vcruntimed.lib>"
    "$<$<CONFIG:Debug>:ucrtd.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:vcruntime.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:ucrt.lib>"
)

add_library(SDL2main STATIC
    "${SDL2_SOURCE_DIR}/src/main/windows/SDL_windows_main.c"
)
target_include_directories(SDL2main PUBLIC "${SDL2_SOURCE_DIR}/include")
target_compile_definitions(SDL2main PRIVATE
    _WINDOWS
    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<NOT:$<CONFIG:Debug>>:NDEBUG>"
)


foreach(target SDL2 SDL2main)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED TRUE
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W3    # WarningLevel
            /GS-   # BufferSecurityCheck false
            
            "$<$<CONFIG:Debug>:/Od>"
            "$<$<NOT:$<CONFIG:Debug>>:/Ob1>" # InlineFunctionExpansion
            
            "$<$<CONFIG:Debug>:/MDd>"
            "$<$<NOT:$<CONFIG:Debug>>:/MD>"
        )

        target_link_options(${target} PRIVATE
            /IGNORE:4099 # OmitDefaultLibName
            "$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>" # OptimizeReferences
            "$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>" # EnableCOMDATFolding
        )
    else()
        target_compile_options(${target} PRIVATE
            -w # Disable all warnings
            "$<$<CONFIG:Debug>:-O0>"
            "$<$<NOT:$<CONFIG:Debug>>:-O2>"
        )
    endif()
endforeach()