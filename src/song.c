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

#include "song.h"

#include <assert.h>

void SongInit(song_t* song)
{
    assert(song != NULL);

    strcpy(song->title, "TITLE");
    strcpy(song->artist, "ARTIST NAME");
    strcpy(song->album, "ALBUM NAME");
    song->song_path_offset = NULL;
    //song->audio_data = NULL;
    song->file = NULL;
    song->audio_data_size = 0;
    song->song_type = SONG_TYPE_INVALID;
    song->sample_rate = 0;
    song->channel_count = 0;
    song->bps = 0;
}

void SongFreeAudioData(song_t* song)
{
    assert(song != NULL);
    assert(song->file != NULL);
    
    fclose(song->file);
    song->file = NULL;
}