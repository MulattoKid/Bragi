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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "macros.h"
#include "song.h"

#include <stdint.h>

typedef struct
{
    char*    song_paths;
    song_t*  songs;
    song_t*  songs_shuffled;
    uint64_t song_count;
    uint64_t current_song_index;
} playlist_t;

typedef enum
{
    PLAYLIST_ERROR_NO = 0, // No error
    PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE = 1, // Unable to open the path supplied
    PLAYLIST_ERROR_EMPTY = 2 // Playlist file doesn't contain any supported audio files
} playlist_error_e;

void             PlaylistInit(playlist_t* playlist);
playlist_error_e PlaylistLoad(char* playlist_file_path, playlist_t* playlist);
void             PlaylistShuffle(playlist_t* playlist);
void             PlaylistFree(playlist_t* playlist);
playlist_error_e PlaylistGenerate(const char* directory_path, const char* playlist_output_file_path);

#endif