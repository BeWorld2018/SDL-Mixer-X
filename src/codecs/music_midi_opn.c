/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef MUSIC_MID_OPNMIDI

/* This file supports libOPNMIDI music streams */

#include "SDL_mixer_ext.h"

#include "music_midi_opn.h"

#include <opnmidi.h>
#include "OPNMIDI/gm_opn_bank.h"

#include <stdio.h>

/* Global OPNMIDI flags which are applying on initializing of MIDI player with a file */
static int opnmidi_scalemod     = 0;
static int opnmidi_logVolumes   = 0;
static int opnmidi_volumeModel  = 0;
static char opnmidi_customBankPath[2048] = "";

/* This structure supports OPNMIDI-based MIDI music streams */
typedef struct
{
    int play_count;
    struct OPN2_MIDIPlayer *opnmidi;
    int playing;
    int volume;
    int gme_t_sample_rate;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;

    char *mus_title;
    char *mus_artist;
    char *mus_album;
    char *mus_copyright;
} OpnMIDI_Music;


int OPNMIDI_getScaleMod()
{
    return opnmidi_scalemod;
}

void OPNMIDI_setScaleMod(int sc)
{
    opnmidi_scalemod = sc;
}

int OPNMIDI_getLogarithmicVolumes()
{
    return opnmidi_logVolumes;
}

void OPNMIDI_setLogarithmicVolumes(int vm)
{
    opnmidi_logVolumes = vm;
}

int OPNMIDI_getVolumeModel()
{
    return opnmidi_volumeModel;
}

void OPNMIDI_setVolumeModel(int vm)
{
    opnmidi_volumeModel = vm;
    if(vm < 0)
        opnmidi_volumeModel = 0;
}

void OPNMIDI_setLoops(void *music_p, int loop)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    if(music)
        opn2_setLoopEnabled(music->opnmidi, (loop >=0 ? 0 : 1));
}

void OPNMIDI_setDefaults()
{
    opnmidi_scalemod    = 0;
}

/*
 * Initialize the OPNMIDI player, with the given mixer settings
 * This function returns 0, or -1 if there was an error.
 */

/* Set the volume for a OPNMIDI stream */
static void OPNMIDI_setvolume(void *music_p, int volume)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    if(music)
    {
        music->volume = (int)round(128.0*sqrt(((double)volume)*(1.0/128.0) ));
    }
}

void OPNMIDI_setCustomBankFile(const char *bank_wonp_path)
{
    if(bank_wonp_path)
        strcpy(opnmidi_customBankPath, bank_wonp_path);
    else
        opnmidi_customBankPath[0] = '\0';
}

static void OPNMIDI_delete(void *music_p);

static OpnMIDI_Music *OPNMIDI_LoadSongRW(SDL_RWops *src)
{
    if(src != NULL)
    {
        void *bytes=0;
        long filesize = 0;
        int err = 0;
        Sint64 length=0;
        size_t bytes_l;
        unsigned char byte[1];
        OpnMIDI_Music *music = NULL;

        music = (OpnMIDI_Music *)SDL_calloc(1, sizeof(OpnMIDI_Music));

        music->stream = SDL_NewAudioStream(AUDIO_S16, 2, music_spec.freq,
                                           music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->stream) {
            OPNMIDI_delete(music);
            return NULL;
        }

        music->buffer_size = music_spec.samples * sizeof(Sint16) * 2/*channels*/ * music_spec.channels;
        music->buffer = SDL_malloc(music->buffer_size);
        if (!music->buffer) {
            OPNMIDI_delete(music);
            return NULL;
        }

        length = SDL_RWseek(src, 0, RW_SEEK_END);
        if (length < 0)
        {
            Mix_SetError("OPN2-MIDI: wrong file\n");
            return NULL;
        }

        SDL_RWseek(src, 0, RW_SEEK_SET);
        bytes = SDL_malloc((size_t)length);

        filesize = 0;
        while( (bytes_l = SDL_RWread(src, &byte, sizeof(Uint8), 1)) != 0)
        {
            ((unsigned char*)bytes)[filesize] = byte[0];
            filesize++;
        }

        if (filesize == 0)
        {
            Mix_SetError("OPN2-MIDI: wrong file\n");
            SDL_free(bytes);
            return NULL;
        }

        music->opnmidi = opn2_init( music_spec.freq );
        if(opnmidi_customBankPath[0] != '\0')
            err = opn2_openBankFile(music->opnmidi, (char*)opnmidi_customBankPath);
        else
            err = opn2_openBankData(music->opnmidi, g_gm_opn2_bank, sizeof(g_gm_opn2_bank));
        if( err < 0 )
        {
            Mix_SetError("OPN2-MIDI: %s", opn2_errorInfo(music->opnmidi));
            SDL_free(bytes);
            OPNMIDI_delete(music);
            return NULL;
        }

        opn2_setLogarithmicVolumes( music->opnmidi, opnmidi_logVolumes );
        opn2_setVolumeRangeModel( music->opnmidi, opnmidi_volumeModel );
        opn2_setNumCards( music->opnmidi, 4 );

        err = opn2_openData( music->opnmidi, bytes, (unsigned long)filesize);
        SDL_free(bytes);

        if(err != 0)
        {
            Mix_SetError("OPN2-MIDI: %s", opn2_errorInfo(music->opnmidi));
            OPNMIDI_delete(music);
            return NULL;
        }

        music->volume                 = MIX_MAX_VOLUME;
        music->mus_title = NULL;
        music->mus_artist = NULL;
        music->mus_album = NULL;
        music->mus_copyright = NULL;

        return music;
    }
    return NULL;
}

/* Load OPNMIDI stream from an SDL_RWops object */
static void *OPNMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    OpnMIDI_Music *adlmidiMusic;

    adlmidiMusic = OPNMIDI_LoadSongRW(src);
    if (!adlmidiMusic)
        return NULL;
    if( freesrc )
        SDL_RWclose(src);

    return adlmidiMusic;
}

/* Start playback of a given Game Music Emulators stream */
static int OPNMIDI_play(void *music_p, int play_counts)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    opn2_positionRewind(music->opnmidi);
    music->play_count = play_counts;
    opn2_setLoopEnabled(music->opnmidi, (play_counts < 0));
    return 0;
}

static int OPNMIDI_playSome(void *context, void *data, int bytes, SDL_bool *done)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)context;
    int filled, gottenLen, amount;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    gottenLen = opn2_play(music->opnmidi, (bytes / 2), (short*)music->buffer);
    if (gottenLen <= 0) {
        *done = SDL_TRUE;
        return 0;
    }

    amount = gottenLen * 2;
    if (amount > 0) {
        if (SDL_AudioStreamPut(music->stream, music->buffer, amount) < 0) {
            return -1;
        }
    } else {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            opn2_positionRewind(music->opnmidi);
        }
    }

    return 0;
}

/* Play some of a stream previously started with OPNMIDI_play() */
static int OPNMIDI_playAudio(void *music_p, void *stream, int len)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    return music_pcm_getaudio(music_p, stream, len, music->volume, OPNMIDI_playSome);
}

/* Close the given Game Music Emulators stream */
static void OPNMIDI_delete(void *music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    if(music)
    {
        /* META-TAGS */
        if(music->mus_title)
            SDL_free(music->mus_title);
        if(music->mus_artist)
            SDL_free(music->mus_artist);
        if(music->mus_album)
            SDL_free(music->mus_album);
        if(music->mus_copyright)
            SDL_free(music->mus_copyright);
        /* META-TAGS */

        if(music->opnmidi) {
            opn2_close( music->opnmidi );
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        SDL_free( music );
    }
}

/* Jump (seek) to a given position (time is in seconds) */
static int OPNMIDI_jump_to_time(void *music_p, double time)
{
    OpnMIDI_Music *music = (OpnMIDI_Music*)music_p;
    opn2_positionSeek(music->opnmidi, time);
    return 0;
}

static double OPNMIDI_currentPosition(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return opn2_positionTell(music->opnmidi);
}


static double OPNMIDI_songLength(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return opn2_totalTimeLength(music->opnmidi);
}

static double OPNMIDI_loopStart(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return opn2_loopStartTime(music->opnmidi);
}

static double OPNMIDI_loopEnd(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    return opn2_loopEndTime(music->opnmidi);
}

static double OPNMIDI_loopLength(void* music_p)
{
    OpnMIDI_Music *music = (OpnMIDI_Music *)music_p;
    if(music)
    {
        double start = opn2_loopStartTime(music->opnmidi);
        double end = opn2_loopEndTime(music->opnmidi);
        if(start >= 0 && end >= 0)
            return (end - start);
    }
    return -1;
}

//int OPNMIDI_init2(Mix_MusicInterface *codec, SDL_AudioSpec *mixerfmt)
//{
//    mixer = *mixerfmt;

//    initMusicInterface(codec);

//    codec->isValid = 1;

//    codec->capabilities     = music_interface_default_capabilities;

//    codec->open             = OPNMIDI_new_RW;
//    codec->openEx           = music_interface_dummy_cb_openEx;
//    codec->close            = OPNMIDI_delete;

//    codec->play             = OPNMIDI_play;
//    codec->pause            = music_interface_dummy_cb_void_1arg;
//    codec->resume           = music_interface_dummy_cb_void_1arg;
//    codec->stop             = OPNMIDI_stop;

//    codec->isPlaying        = OPNMIDI_playing;
//    codec->isPaused         = music_interface_dummy_cb_int_1arg;

//    codec->setLoops         = OPNMIDI_setLoops;
//    codec->setVolume        = OPNMIDI_setvolume;

//    codec->jumpToTime       = OPNMIDI_jump_to_time;
//    codec->getCurrentTime   = OPNMIDI_currentPosition;
//    codec->getTimeLength    = OPNMIDI_songLength;

//    codec->getLoopStartTime = OPNMIDI_loopStart;
//    codec->getLoopEndTime   = OPNMIDI_loopEnd;
//    codec->getLoopLengthTime= OPNMIDI_loopLength;

//    codec->metaTitle        = music_interface_dummy_meta_tag;
//    codec->metaArtist       = music_interface_dummy_meta_tag;
//    codec->metaAlbum        = music_interface_dummy_meta_tag;
//    codec->metaCopyright    = music_interface_dummy_meta_tag;

//    codec->playAudio        = OPNMIDI_playAudio;

//    return(0);
//}

Mix_MusicInterface Mix_MusicInterface_OPNMIDI =
{
    "OPNMIDI",
    MIX_MUSIC_OPNMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    OPNMIDI_new_RW,
    NULL,   /* CreateFromFile */
    OPNMIDI_setvolume,
    OPNMIDI_play,
    NULL,   /* IsPlaying */
    OPNMIDI_playAudio,
    OPNMIDI_jump_to_time,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    OPNMIDI_delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif /*USE_OPN2_MIDI*/

