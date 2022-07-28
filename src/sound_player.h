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

#ifndef SOUND_PLAYER_H
#define SOUND_PLAYER_H

#include "song.h"

#include <windows.h>

typedef enum
{
    SOUND_PLAYER_OP_READY    = 1,
    SOUND_PLAYER_OP_PLAY     = 2,
    SOUND_PLAYER_OP_NEXT     = 3,
    SOUND_PLAYER_OP_PREVIOUS = 4,
    SOUND_PLAYER_OP_PAUSE    = 5,
    SOUND_PLAYER_OP_RESUME   = 6,
    SOUND_PLAYER_OP_SHUFFLE  = 7
}  sound_player_operation_e;

typedef enum
{
    SOUND_PLAYER_LOOP_NO       = 0,
    SOUND_PLAYER_LOOP_PLAYLIST = 1,
    SOUND_PLAYER_LOOP_SINGLE   = 2
} sound_player_loop_e;

typedef enum
{
    SOUND_PLAYER_SHUFFLE_NO = 0,
    SOUND_PLAYER_SHUFFLE_RANDOM = 1
} sound_player_shuffle_e;

// TODO (Daniel): pack properly
typedef struct
{
    HANDLE                   event;
    sound_player_operation_e ui_next_operation;
    
    HANDLE                   mutex; // Required to be locked before accessing below members
    song_t*                  song;
    HWAVEOUT                 audio_device;
    sound_player_loop_e      loop_state;
    sound_player_shuffle_e   shuffle_state;
    uint8_t                  playlist_current_changed;
    uint8_t                  error_message_changed;
    char                     playlist_next_file_path[MAX_PATH];
    char                     playlist_current_file_path[MAX_PATH];
    char                     error_message[MAX_PATH];

    HANDLE                   current_playback_buffer_mutex; // Required to be locked before accessing below members
    byte_t*                  current_playback_buffer;
    uint64_t                 current_playback_buffer_size;
} sound_player_shared_data_t;

typedef struct
{
    HWAVEOUT                  audio_device;
    FILE*                     file;
    uint64_t                  file_size;
    uint32_t                  sample_rate;
    uint8_t                   channel_count;
    uint8_t                   bps; // Bytes per sample
} playback_data_t;

typedef struct
{
    HANDLE event;
    int32_t callback_count_atomic;
} callback_data_t;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
DWORD WINAPI SoundPlayerThreadProc(_In_ LPVOID lpParameter);

#endif