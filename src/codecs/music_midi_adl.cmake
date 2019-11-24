option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support" ON)
if(USE_MIDI_ADLMIDI)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(ADLMIDI REQUIRED)
        message("ADLMIDI: [${ADLMIDI_FOUND}] ${ADLMIDI_INCLUDE_DIRS} ${ADLMIDI_LIBRARIES}")
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(ADLMIDI_LIBRARIES ADLMIDI)
        else()
            find_library(ADLMIDI_LIBRARIES NAMES ADLMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        if(ADLMIDI_LIBRARIES)
            set(ADLMIDI_FOUND 1)
        endif()
        set(ADLMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libADLMIDI/include")
    endif()

    if(ADLMIDI_FOUND)
        message("== using ADLMIDI ==")
        add_definitions(-DMUSIC_MID_ADLMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${ADLMIDI_INCLUDE_DIRS})
        list(APPEND SDLMixerX_LINK_LIBS ${ADLMIDI_LIBRARIES})
    else()
        message("== skipping ADLMIDI ==")
    endif()
endif()

# Keep this file always built as it contains dummies of some public calls to avoid link errors
list(APPEND SDLMixerX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/music_midi_adl.c)
