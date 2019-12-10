/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

/* This file supports reading of ID3 tags of MP3 files with libID3Tag */

#include "music.h"

typedef struct {
    Sint64 begin;
    Sint64 end;
} Id3TagLengthStrip;

struct mp3file_t {
    SDL_RWops *src;
    Sint64 start, length, pos;
};

extern int mp3_read_tags(Mix_MusicMetaTags *out_tags, SDL_RWops *src, struct mp3file_t *file_edges);
extern int read_id3v2_from_mem(Mix_MusicMetaTags *out_tags, Uint8 *data, size_t length);

extern size_t MP3_RWread(struct mp3file_t *fil, void *ptr, size_t size, size_t maxnum);
extern Sint64 MP3_RWseek(struct mp3file_t *fil, Sint64 offset, int whence);

/* vi: set ts=4 sw=4 expandtab: */
