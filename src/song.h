/**
 * Copyright (c) 2022 - 2022, Daniel Fedai Larsen.
 *
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SONG_H
#define SONG_H

#include "macros.h"

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef enum
{
    SONG_ERROR_NO = 0,
    SONG_ERROR_UNABLE_TO_OPEN_FILE = 1,
    SONG_ERROR_INVALID_FILE = 2
} song_error_e;

typedef enum
{
    SONG_TYPE_INVALID = 0,
    SONG_TYPE_WAV = 1,
    SONG_TYPE_FLAC = 2
} song_type_e;

// TODO (Daniel): split so that each song in a playlist doesn't require this much memory (wasteful/thrashy)
typedef struct
{
    char title[MAX_PATH];
    char artist[MAX_PATH];
    char album[MAX_PATH];
    char* song_path_offset; // Not allocated, just an offset into an array
    //byte_t* audio_data;
    FILE* file;
    uint64_t file_size;
    uint64_t audio_data_size;
    song_type_e song_type;
    uint16_t sample_rate;
    uint8_t channel_count;
    uint8_t bps; // Bytes per sample
} song_t;

void SongInit(song_t* song);
void SongFreeAudioData(song_t* song);

#endif