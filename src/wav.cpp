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
        if (strncmp((char*)((byte_t*)wav_header_packed + index), "JUNK", 4) == 0)
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
            printf("WAV subchunk type '%s' not supported\n", (char*)wav_header_packed);
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

    // Assign WAV info and data to song
    song->audio_data = (byte_t*)malloc(wav_data_subchunk_packed->subchunk2_size);
    memcpy(song->audio_data, wav_audio_data, wav_data_subchunk_packed->subchunk2_size);
    song->audio_data_size = wav_data_subchunk_packed->subchunk2_size;
    song->sample_rate = wav_header_packed->sample_rate;
    song->channel_count = wav_header_packed->channel_count;
    song->bits_per_sample = wav_header_packed->bits_per_sample;

    // Cleanxup
    free(wav_header_packed);

    return SONG_ERROR_NO;
}