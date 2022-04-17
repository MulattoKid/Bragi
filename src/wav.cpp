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

#include "wav.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

song_error_e WAVLoad(song_t* song)
{
    assert(song != NULL);
    assert(song->song_path_offset != NULL);
    assert(song->audio_data == NULL);

    // Load WAV file into memory
    FILE* wav_file = fopen(song->song_path_offset, "rb");
    if (wav_file == NULL)
    {
        printf("Failed to open %s : \"%s\"\n", song->song_path_offset, strerror(errno));
        return SONG_ERROR_UNABLE_TO_OPEN_FILE;
    }
    fseek(wav_file, 0, SEEK_END);
    long wav_file_size = ftell(wav_file);
    fseek(wav_file, 0, SEEK_SET);
    wav_header_packed_t* wav_header_packed = (wav_header_packed_t*)malloc(wav_file_size);
    fread((void*)wav_header_packed, wav_file_size, 1, wav_file);
    fclose(wav_file);

    // Find 'data' chunk in WAV file
    bool found_data_chunk = 0;
    int32_t index = sizeof(wav_header_packed_t);
    while (index < wav_file_size)
    {
        // This first 4 bytes are always the subchunk's ID
        if (strncmp((char*)((byte_t*)wav_header_packed + index), "data", 4) == 0)
        {
            found_data_chunk = 1;
            break;
        }
        uint32_t new_index = index + 4;

        // The next 4 bytes are always the subchunk's size
        uint32_t subchunk_size = (uint32_t)*((byte_t*)wav_header_packed + new_index);
        new_index += 4;

        // Determine where this subchunk ends
        if (strncmp((char*)((byte_t*)wav_header_packed + index), "LIST", 4) == 0)
        {}
        else if (strncmp((char*)((byte_t*)wav_header_packed + index), "JUNK", 4) == 0)
        {
            if (subchunk_size % 2 != 0)
            {
                subchunk_size += 1;
            }
        }
        else if (strncmp((char*)((byte_t*)wav_header_packed + index), "Fake", 4) == 0)
        {}
        else
        {
            printf("WAV subchunk type '%s' not supported\n", (char*)(wav_header_packed + index));
            assert(0);
        }
        new_index += subchunk_size;

        index = new_index;
    }

    // Verify WAV file
    if ((strncmp(wav_header_packed->chunk_id, "RIFF", 4) != 0) ||
        (strncmp(wav_header_packed->chunk_format, "WAVE", 4) != 0) ||
        (strncmp(wav_header_packed->subchunk1_id, "fmt ", 4) != 0) ||
        (found_data_chunk == false))
    {
        free(wav_header_packed);
        return SONG_ERROR_INVALID_FILE;
    }

    wav_subchunk_header_packed_t* wav_data_subchunk_packed = (wav_subchunk_header_packed_t*)((byte_t*)wav_header_packed + index);
    byte_t* wav_audio_data = (byte_t*)wav_data_subchunk_packed + sizeof(wav_subchunk_header_packed_t);
    
    // TODO: Optimize
    // TODO: move sample-rate conversion elsewhere so that it's no longer WAV specific.
    // Assign WAV info and data to song
#if 0
    song->audio_data = (byte_t*)malloc(wav_data_subchunk_packed->subchunk2_size);
    memcpy(song->audio_data, wav_audio_data, wav_data_subchunk_packed->subchunk2_size);
    song->audio_data_size = wav_data_subchunk_packed->subchunk2_size;
    song->sample_rate = wav_header_packed->sample_rate;
    song->channel_count = wav_header_packed->channel_count;
    song->bits_per_sample = wav_header_packed->bits_per_sample;
#else
    int32_t bps = wav_header_packed->bits_per_sample / 8;
    assert(bps == 2);
    int32_t sample_count = wav_data_subchunk_packed->subchunk2_size / bps;
    assert(sample_count % 2 == 0);
    int32_t halfway_index = sample_count / 2;
    // Deinterleave
    byte_t* deinterleaved_wav_audio_data = (byte_t*)malloc(sample_count * bps);
    int32_t deinterleaved_index0 = 0, deinterleaved_index1 = halfway_index;
    for (int32_t i = 0; i < sample_count; i++)
    {
        if (i % 2 == 0)
        {
            memcpy(deinterleaved_wav_audio_data + (deinterleaved_index0 * bps), wav_audio_data + (i * bps), bps);
            deinterleaved_index0++;
        }
        else
        {
            memcpy(deinterleaved_wav_audio_data + (deinterleaved_index1 * bps), wav_audio_data + (i * bps), bps);
            deinterleaved_index1++;
        }
    }
    // Perform sample-rate conversion
    float slow_down_factor = 0.9f;
    float resampling_factor = (1.0f - slow_down_factor) + 1.0f;
    assert(slow_down_factor < 1.0f);
    assert(resampling_factor >= 1.0f);
    assert(resampling_factor < 2.0f);
    int32_t input_rate = wav_header_packed->sample_rate;
    int32_t final_rate = input_rate * resampling_factor;
    // Find greatest common divisor
    // https://en.wikipedia.org/wiki/Euclidean_algorithm#Implementations
    int32_t a = input_rate;
    int32_t b = final_rate;
    while (b != 0)
    {
        int32_t t = b;
        b = a % b;
        a = t;
    }
    int32_t L = final_rate / a; // Upsampling factor
    int32_t M = input_rate / a; // Decimation factor
    // Upsample by zero-stuffing
    int32_t sample_count_upsampled = sample_count * (L + 1);
    byte_t* upsampled_audio_data = (byte_t*)malloc(sample_count_upsampled * bps);
    memset(upsampled_audio_data, 0, sample_count_upsampled * bps);
    for (int32_t i = 0; i < sample_count; i++)
    {
        memcpy(upsampled_audio_data + (i * (L + 1) * bps), deinterleaved_wav_audio_data + (i * bps), bps);
    }
    // Create filter
    const float pi = 3.14159265359f;
    const int32_t filter_length = 64;
    int32_t filter_shift = filter_length / 2;
    int32_t filter_sample_rate = input_rate * (L + 1);
    float filter_sample_delta = 1.0f / (float)filter_sample_rate;
    float cutoff_freq = wav_header_packed->sample_rate / 2;
    float hamming_constant = 0.53836f;
    float filter[filter_length];
    float filter_sum = 0.0f;
    for (int32_t i = 0; i < filter_length; i++)
    {
        int32_t sample_index_shifted = i;
        int32_t sample_index = i - filter_shift;
        float sample_time = float(sample_index) * filter_sample_delta;

        float sinc_value = 2.0f * cutoff_freq;
        if (sample_index != 0)
        {
            sinc_value = sinf(2.0 * pi * cutoff_freq * sample_time) / (pi * sample_time);
        }
        float hamming_value = hamming_constant - ((1.0 - hamming_constant) * cosf((2.0 * pi * float(sample_index_shifted)) / float(filter_length - 1)));
        filter[i] = sinc_value * hamming_value;
        filter_sum += fabs(filter[i]);
    }
    for (int32_t i = 0; i < filter_length; i++)
    {
        filter[i] /= filter_sum;
    }
    // Filter
    assert(bps == 2);
    byte_t* upsampled_audio_data_filtered = (byte_t*)malloc(sample_count_upsampled * bps);
    for (int32_t i = 0; i < sample_count_upsampled; i++)
    {
        float filtered_sample = 0.0f;
        int32_t filter_length_for_sample = min(filter_length, sample_count_upsampled - i);
        for (int32_t j = 0; j < filter_length_for_sample; j++)
        {
            filtered_sample += (float)((int16_t*)upsampled_audio_data)[i + j] * filter[j];
        }
        ((int16_t*)upsampled_audio_data_filtered)[i] = filtered_sample;
    }
    // Decimate
    int32_t sample_count_final = sample_count_upsampled / M;
    byte_t* upsampled_audio_data_final = (byte_t*)malloc(sample_count_final * bps);
    for (int32_t i = 0; i < sample_count_final; i++)
    {
        ((int16_t*)upsampled_audio_data_final)[i] = ((int16_t*)upsampled_audio_data_filtered)[i * M];
    }
    // Interleave 
    byte_t* interleaved_upsampled_audio_data_final = (byte_t*)malloc(sample_count_final * bps);
    assert(sample_count_final % 2 == 0);
    halfway_index = sample_count_final / 2;
    deinterleaved_index0 = 0, deinterleaved_index1 = halfway_index;
    for (int32_t i = 0; i < sample_count_final; i++)
    {
        if (i % 2 == 0)
        {
            memcpy(interleaved_upsampled_audio_data_final + (i * bps), upsampled_audio_data_final + (deinterleaved_index0 * bps), bps);
            deinterleaved_index0++;
        }
        else
        {
            memcpy(interleaved_upsampled_audio_data_final + (i * bps), upsampled_audio_data_final + (deinterleaved_index1 * bps), bps);
            deinterleaved_index1++;
        }
    }

    // Assign WAV info and data to song
    song->audio_data = (byte_t*)malloc(sample_count_final * bps);
    memcpy(song->audio_data, interleaved_upsampled_audio_data_final, sample_count_final * bps);
    song->audio_data_size = sample_count_final * bps;
    song->sample_rate = wav_header_packed->sample_rate;
    song->channel_count = wav_header_packed->channel_count;
    song->bits_per_sample = wav_header_packed->bits_per_sample;

    free(interleaved_upsampled_audio_data_final);
    free(upsampled_audio_data_final);
    free(upsampled_audio_data_filtered);
    free(upsampled_audio_data);
    free(deinterleaved_wav_audio_data);
#endif

    // Cleanxup
    free(wav_header_packed);

    return SONG_ERROR_NO;
}