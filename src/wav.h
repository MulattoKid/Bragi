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

#include <stdint.h>

// http://soundfile.sapp.org/doc/WaveFormat/
// https://www.daubnet.com/en/file-format-riff
PACK
(
struct wav_header_packed_t
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
}
);

PACK
(
struct wav_subchunk_header_packed_t
{
    char    subchunk2_id[4]; // Must equal 'data'
    int32_t subchunk2_size; // sample_count * channel_count * (bits_per_sample / 8) = total_sample_data_size
}
);

// TODO (Daniel): pack properly
struct wav_t
{
    byte_t* base_pointer;
    byte_t* audio_data;
    int32_t audio_data_size;
    int32_t sample_rate;
    int32_t bytes_per_second; // sample_rate * channel_count * (bits_per_sample / 8) = bytes_per_second
    int16_t channel_count;
    int16_t bits_per_sample; // One sample per channel
    int16_t bytes_per_sample_all_channels; // channel_count * (bits_per_sample / 8)
};

song_error_e WAVLoad(song_t* song);


#endif WAV_H