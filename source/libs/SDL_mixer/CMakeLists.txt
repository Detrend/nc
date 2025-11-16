set(MIXER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(SDL_mixer SHARED)

target_sources(SDL_mixer PRIVATE
    "${MIXER_SOURCE_DIR}/src/effect_position.c"
    "${MIXER_SOURCE_DIR}/src/effect_stereoreverse.c"
    "${MIXER_SOURCE_DIR}/src/effects_internal.c"
    "${MIXER_SOURCE_DIR}/src/mixer.c"
    "${MIXER_SOURCE_DIR}/src/music.c"
    "${MIXER_SOURCE_DIR}/src/utils.c"
    "${MIXER_SOURCE_DIR}/src/codecs/load_aiff.c"
    "${MIXER_SOURCE_DIR}/src/codecs/load_voc.c"
    "${MIXER_SOURCE_DIR}/src/codecs/mp3utils.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_cmd.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_drflac.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_flac.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_fluidsynth.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_gme.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_minimp3.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_modplug.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_mpg123.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_nativemidi.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_ogg.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_ogg_stb.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_opus.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_timidity.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_wav.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_wavpack.c"
    "${MIXER_SOURCE_DIR}/src/codecs/music_xmp.c"
)

target_include_directories(SDL_mixer
    PUBLIC  "${MIXER_SOURCE_DIR}/include"
    PRIVATE "${MIXER_SOURCE_DIR}/src"
    PRIVATE "${MIXER_SOURCE_DIR}/src/codecs"
)

target_link_libraries(SDL_mixer PRIVATE
    SDL2
    winmm.lib
)

target_compile_definitions(SDL_mixer PRIVATE
    DLL_EXPORT
    _WINDOWS
    MUSIC_WAV
    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<NOT:$<CONFIG:Debug>>:NDEBUG;_CRT_SECURE_NO_WARNINGS>"
)

if(MSVC)
    target_compile_options(SDL_mixer PRIVATE
        /W3 # Warning Level 3
        
        "$<$<CONFIG:Debug>:/Od>"
        "$<$<NOT:$<CONFIG:Debug>>:/O2>"
        
        "$<$<CONFIG:Debug>:/MDd>"
        "$<$<NOT:$<CONFIG:Debug>>:/MD>"
    )
    
    target_link_options(SDL_mixer PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>"
        "$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>"
    )
else()
    target_compile_options(SDL_mixer PRIVATE
        -w
        "$<$<CONFIG:Debug>:-O0>"
        "$<$<NOT:$<CONFIG:Debug>>:-O2>"
    )
endif()