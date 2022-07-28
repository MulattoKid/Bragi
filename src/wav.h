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

#ifndef WAV_H
#define WAV_H

#include "macros.h"
#include "song.h"
#include "windows_audio.h"

#include <stdint.h>

// http://soundfile.sapp.org/doc/WaveFormat/
// https://www.daubnet.com/en/file-format-riff
PACK
(
typedef struct
{
    char    chunk_id[4]; // Must equal 'RIFF'
    int32_t chunk_size;
    char    chunk_format[4]; // Must equal 'WAVE'

    char    subchunk1_id[4]; // Must equal 'fmt '
    int32_t subchunk1_size;
    int16_t audio_format; // Must equal 1 == PCM
    int16_t channel_count;
    int32_t sample_rate;
    int32_t bytes_per_second; // sample_rate * channel_count * (bits_per_sample / 8)
    int16_t bytes_per_sample_all_channels; // channel_count * (bits_per_sample / 8)
    int16_t bits_per_sample; // Either 8 or 16
} wav_header_packed_t
);

PACK
(
typedef struct
{
    char    subchunk2_id[4]; // Must equal 'data'
    int32_t subchunk2_size; // sample_count * channel_count * (bits_per_sample / 8) = total_sample_data_size
} wav_subchunk_header_packed_t
);

// TODO (Daniel): pack properly
typedef struct
{
    byte_t* base_pointer;
    byte_t* audio_data;
    int32_t audio_data_size;
    int32_t sample_rate;
    int16_t channel_count;
    uint8_t bps; // Bytes per sample
} wav_t;

song_error_e WAVLoadHeader(song_t* song);
uint32_t     WAVLoadData(playback_data_t* audio_thread_data, uint64_t output_size, byte_t* output);

#endif WAV_H