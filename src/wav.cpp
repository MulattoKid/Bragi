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

song_error_e WAVLoadHeader(song_t* song)
{
    assert(song != NULL);
    assert(song->song_path_offset != NULL);
    //assert(song->audio_data == NULL);

    // Open WAV file
    FILE* wav_file = fopen(song->song_path_offset, "rb");
    if (wav_file == NULL)
    {
        printf("Failed to open %s : \"%s\"\n", song->song_path_offset, strerror(errno));
        return SONG_ERROR_UNABLE_TO_OPEN_FILE;
    }

    // Determine WAV file size
    fseek(wav_file, 0, SEEK_END);
    long wav_file_size = ftell(wav_file);
    fseek(wav_file, 0, SEEK_SET);

    // Read in WAV file header
    wav_header_packed_t wav_header_packed;
    fread((void*)(&wav_header_packed), sizeof(wav_header_packed_t), 1, wav_file);

    // Find 'data' chunk in WAV file
    bool found_data_subchunk = 0;
    uint32_t data_subchunk_size = 0;
    int32_t index = sizeof(wav_header_packed_t);
    char subchunk_id[4];
    while (index < wav_file_size)
    {
        // The next 4 bytes are always the subchunk's ID
        fread(subchunk_id, 4, 1, wav_file);
        
        // The next 4 bytes are always the subchunk's size
        uint32_t subchunk_size;
        fread(&subchunk_size, 4, 1, wav_file);

        // Determine where this subchunk ends
        if (strncmp(subchunk_id, "data", 4) == 0)
        {
            found_data_subchunk = 1;
            data_subchunk_size = subchunk_size;
            break;
        }
        else if (strncmp(subchunk_id, "JUNK", 4) == 0)
        {
            if (subchunk_size % 2 != 0)
            {
                subchunk_size += 1;
            }
        }
        else if (strncmp(subchunk_id, "LIST", 4) == 0)
        {}
        else if (strncmp(subchunk_id, "Fake", 4) == 0)
        {}
        else
        {
            printf("WAV subchunk type '%s' not supported\n", subchunk_id);
            assert(0);
        }
        fseek(wav_file, subchunk_size, SEEK_CUR);

        index += 4 + 4 + subchunk_size;
    }

    // Verify WAV file
    if ((strncmp(wav_header_packed.chunk_id, "RIFF", 4) != 0) ||
        (strncmp(wav_header_packed.chunk_format, "WAVE", 4) != 0) ||
        (strncmp(wav_header_packed.subchunk1_id, "fmt ", 4) != 0) ||
        (found_data_subchunk == false))
    {
        fclose(wav_file);
        return SONG_ERROR_INVALID_FILE;
    }
    
    // Assign WAV info and data to song
    song->file = wav_file;
    song->file_size = wav_file_size;
    song->audio_data_size = data_subchunk_size;
    song->sample_rate = wav_header_packed.sample_rate;
    song->channel_count = wav_header_packed.channel_count;
    song->bps = wav_header_packed.bits_per_sample / 8;

    return SONG_ERROR_NO;
}

uint32_t WAVLoadData(playback_data_t* audio_thread_data, uint64_t output_size, byte_t* output)
{
    assert(audio_thread_data != NULL);
    assert(output_size > 0);
    assert(output != NULL);

    // Determine how much to read
    long file_offset = ftell(audio_thread_data->file);
    long remaining_bytes = audio_thread_data->file_size - file_offset;
    long size_to_read = remaining_bytes < output_size ? remaining_bytes : output_size;
    uint32_t total_bytes_per_sample_all_channels = audio_thread_data->bps * audio_thread_data->channel_count;
    uint32_t total_samples_that_fit = size_to_read / total_bytes_per_sample_all_channels;
    size_to_read = total_samples_that_fit * audio_thread_data->channel_count * audio_thread_data->bps;
    
    // Read
    fread(output, size_to_read, 1, audio_thread_data->file);

    return size_to_read;
}