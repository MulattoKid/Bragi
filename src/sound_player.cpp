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

/**
 * //////////////////////////
 * Sound Player Specification
 * //////////////////////////
 * 
 * 
 * Shared Data
 * The sound player shares data with the main thread. Through this it will send and receive data to initiate
 * certain actions, and update various information.
 *  - Event: this is used to signal to the sound player that a new operation has occured, and that it must be
 *           handled approrpiately. This event can be set both from the main thread and the sound player's own
 *           callback function.
 *  - Mutex: this controls access to the shared data. It must always be locked before accessing the shared data
 *           (except the Event).
 *  - The rest are self-explanatory.
 * 
 * 
 * Callback
 * The callback is connected to a Windows audio device, and can be invoked in three instances: 1) the device was
 * opened, 2) the device was closed, or 3) it finished playback. We are only interested in the last one. When the
 * callback is invoked, and it was caused by playback finishing, it will check if the sound player is ready
 * to receive a new operation, and if it is, set the next operation to NEXT and signal the Event.
 * 
 * 
 * Operation
 * When the Event is signaled, it means that the sound player has recevied a new operation to perform (see the
 * sound_player_operation_e enum):
 *  - BUSY:     This means that the sound player is already busy with some other operation, and a new one cannot
 *              be set. The reason this is necessary is that when closing the audio device, the callback function
 *              will be invoked with WOM_DONE. For the callback to be able to correctly read the shared data, the
 *              mutex must be unlocked before the callback is invoked. When this happens, the main thread might
 *              also try locking the mutex and updating the next operation. This would be illegal, as an operation
 *              is already in progress. To avoid such a scenario, the BUSY operation is set, and if the main thread
 *              sees this operation as the one currently in progress, it will not alter the operation.
 *  - READY:    This means that the sound player is ready to receive a new operation, and is it's default state.
 *              operation.
 *  - PLAY:     This means that the sound player will start playing from a new playlist.
 *  - NEXT:     This means that the sound player will start playing the next song in the playlist. If the end of the
 *              playlist is reached, the loop-state determines what happens.
 *  - PREVIOUS: This means that the sound player will start playing the previous song in the playlist. If the start
 *              of the playlist is reached, the loop-state determines what happens.
 *  - PAUSE:    This means that playback of the current song will pause.
 *  - RESUME:   This means that playback of the current song will resume.
 *  - SHUFFLE:  This means that the current playlist will be (re-)shuffled.
 * 
 * PLaying a New Playlist
 * Playing a new playlist includes 5 steps:
 *  1) Try to load the playlist file.
 *   a) If this fails, the current playlist isn't updated, and no change is observed.
 *   b) If this succeeds, continue to step 2.
 * 2) Try loading the first song in the playlist.
 *  a) If this fails, the current playlist is still updated to the new playlist, but the current song being played
 *     (if any) continues playing.
 *  b) If this succeeds, continue to step 3.
 * 3) Check if the Windows audio device can play the file format.
 *  a) If this fails, the current playlist is still updated to the new playlist, but the current song being played
 *     (if any) continues playing.
 *  b) If this succeeds, continue to step 4.
 * 4) Stop playback of the current song if one is playing.
 * 5) Start playback of the first song in the playlist.
 * 
 * 
 * Playing the Next WAV File in a Playlist
 * Playing the next WAV file in a playlist consists of N steps:
 * 1) Select the next WAV file to be played. If the end of the playlist is reached, the loop-state determines what happens:
 *   a) LOOP_NO: playback stops.
 *   b) LOOP:    the playlist will restart.
 *    If LOOP_SINGLE is used, then the same WAV file will be reloaded and playback start from the beginning.
 * 2) Try loading the next WAV file.
 *   a) If this fails, the current song being played (if any) continues playing.
 *   b) If this succeeds, continue to step 3.
 * 3) Check if the Windows audio device can play the file format.
 *  a) If this fails, the current song being played (if any) continues playing.
 *  b) If this succeeds, continue to step 3.
 * 4) Stop playback of the current song if one is playing.
 * 5) Start playback of the next WAV file.
 * 
*/

#include "flac.h"
#include "playlist.h"
#include "sound_player.h"
#include "wav.h"
#include "windows_audio.h"
#include "windows_synchronization.h"
#include "windows_thread.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static HANDLE audio_event = NULL;
static const uint8_t audio_buffer_count = 3;
static const uint64_t audio_buffer_size = 8192;
static WAVEHDR audio_headers[audio_buffer_count];
static byte_t audio_buffers[audio_buffer_count][audio_buffer_size];
static uint32_t audio_buffer_data_available_size[audio_buffer_count];
static uint8_t audio_buffer_index = 0;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    // Cast input pointer
    sound_player_shared_data_t* shared_data = (sound_player_shared_data_t*)dwInstance;
    assert(shared_data->mutex != NULL);

    // Handle message
    switch (uMsg)
    {
        case WOM_OPEN: {} break;
        
        case WOM_CLOSE: {} break;

        case WOM_DONE:
        {
            SyncSetEvent(audio_event, __FILE__, __LINE__);
        } break;

        default:
        {
            assert(0);
        } break;
    }
}

static byte_t* prefetch_buffer = NULL;
static byte_t* upsampled_audio_data = NULL;
static byte_t* upsampled_audio_data_proper = NULL;
static byte_t* upsampled_audio_data_filtered = NULL;
static byte_t** upsampled_audio_data_finals = NULL;
static const uint8_t filter_length = 64;
static const uint8_t filter_shift = filter_length / 2;
static const float pi = 3.14159265359f;
static const float hamming_constant = 0.53836f;
static float filter[filter_length];
static float slow_down_factor = 0.8f;
static float resampling_factor = (1.0f - slow_down_factor) + 1.0f;
DWORD WINAPI AudioThreadProc(_In_ LPVOID lpParameter)
{
    // Cast input pointer
    audio_thread_shared_data_t* audio_thread_data = (audio_thread_shared_data_t*)lpParameter;
    
    // If audio_event already exists, recreate it
    if (audio_event != NULL)
    {
        assert(CloseHandle(audio_event) != 0);
    }
    audio_event = CreateEventA(NULL, FALSE, FALSE, "AudioLoaderEvent");
    // Check if any buffers already exists, and if so, free them
    if (prefetch_buffer != NULL)
    {
        free(prefetch_buffer);
    }
    if (upsampled_audio_data != NULL)
    {
        free(upsampled_audio_data);
    }
    if (upsampled_audio_data_proper != NULL)
    {
        free(upsampled_audio_data_proper);
    }
    if (upsampled_audio_data_filtered != NULL)
    {
        free(upsampled_audio_data_filtered);
    }
    if (upsampled_audio_data_finals != NULL)
    {
        for (uint8_t i = 0; i < audio_buffer_count; i++)
        {
            free(upsampled_audio_data_finals[i]);
        }
        free(upsampled_audio_data_finals);
    }

    // Pre-compute some stuff needed for sample-rate conversion
    const uint32_t input_rate = audio_thread_data->sample_rate;
    const uint32_t final_rate = input_rate * resampling_factor;
    // Find greatest common divisor
    // https://en.wikipedia.org/wiki/Euclidean_algorithm#Implementations
    uint32_t a = input_rate;
    uint32_t b = final_rate;
    while (b != 0)
    {
        int32_t t = b;
        b = a % b;
        a = t;
    }
    const uint32_t L = final_rate / a; // Upsampling factor
    const uint32_t M = input_rate / a; // Decimation factor
    const uint32_t bps = audio_thread_data->bps;
    const uint32_t channel_count = audio_thread_data->channel_count;
    const uint32_t bps_all_channels = bps * channel_count;
    const uint32_t max_sample_count_in_audio_buffer = audio_buffer_size / bps_all_channels; // Ensure it fits all samples for a channel
    const uint32_t max_sample_count_in_audio_buffer_all_channels = max_sample_count_in_audio_buffer * channel_count; // Scale up to get number of individual samples
    const uint32_t max_sample_count_upsampled_all_channels = max_sample_count_in_audio_buffer_all_channels * L;
    uint32_t max_sample_count_decimated_all_channels = (max_sample_count_upsampled_all_channels / M);
    max_sample_count_decimated_all_channels -= (max_sample_count_decimated_all_channels % channel_count); // Ensure we can fit entire channels

    // (Re)Allocate buffers
    prefetch_buffer = (byte_t*)malloc(filter_length * audio_thread_data->channel_count * audio_thread_data->bps);
    upsampled_audio_data = (byte_t*)malloc(max_sample_count_upsampled_all_channels * bps);
    upsampled_audio_data_proper = (byte_t*)malloc((max_sample_count_upsampled_all_channels + (filter_length * audio_thread_data->channel_count)) * bps);
    upsampled_audio_data_filtered = (byte_t*)malloc(max_sample_count_upsampled_all_channels * bps);
    upsampled_audio_data_finals = (byte_t**)malloc(audio_buffer_count * sizeof(byte_t*));
    for (uint8_t i = 0; i < audio_buffer_count; i++)
    {
        upsampled_audio_data_finals[i] = (byte_t*)malloc(max_sample_count_decimated_all_channels * bps);
    }

    // Preload first N-1 audio_buffers
    audio_buffer_index = 0;
    for (uint32_t i = 0; i < audio_buffer_count - 1; i++)
    {
        // Load audio data
        audio_buffer_data_available_size[audio_buffer_index] = WAVLoadData(audio_thread_data, audio_buffer_size, audio_buffers[audio_buffer_index]);

        // Ensure there's audio data
        if (audio_buffer_data_available_size[audio_buffer_index] == 0)
        {
            assert(0); // Need to handle this
        }

        // Send audio data to audio device
        audio_headers[audio_buffer_index].lpData = (LPSTR)audio_buffers[audio_buffer_index];
        audio_headers[audio_buffer_index].dwBufferLength = audio_buffer_data_available_size[audio_buffer_index];
        audio_headers[audio_buffer_index].dwBytesRecorded = 0;
        audio_headers[audio_buffer_index].dwUser = NULL;
        audio_headers[audio_buffer_index].dwFlags = 0;
        audio_headers[audio_buffer_index].dwLoops = 0;
        MMRESULT res_mmresult = waveOutPrepareHeader(audio_thread_data->audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
        assert(res_mmresult == MMSYSERR_NOERROR);
        res_mmresult = waveOutWrite(audio_thread_data->audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
        assert(res_mmresult == MMSYSERR_NOERROR);
        audio_buffer_index = (audio_buffer_index + 1) % audio_buffer_count;
    }

    // Load audio data into next audio_buffer, and wait
    while (1)
    {
        // Load next chunk of audio file
        audio_buffer_data_available_size[audio_buffer_index] = WAVLoadData(audio_thread_data, audio_buffer_size, audio_buffers[audio_buffer_index]);
        if (audio_buffer_data_available_size[audio_buffer_index] == 0)
        {
            SyncLockMutex(audio_thread_data->sound_player_mutex, INFINITE, __FILE__, __LINE__);
            *audio_thread_data->sound_player_op = SOUND_PLAYER_OP_NEXT;
            SyncReleaseMutex(audio_thread_data->sound_player_mutex, __FILE__, __LINE__);
            SyncSetEvent(audio_thread_data->sound_player_event, __FILE__, __LINE__);
            return EXIT_SUCCESS;
        }
#if 1
        // Perform sample-rate conversion
        int32_t sample_count_all_channels = audio_buffer_data_available_size[audio_buffer_index] / bps;
        int32_t sample_count = sample_count_all_channels / audio_thread_data->channel_count;
        int32_t sample_count_upsampled = sample_count * L;
        int32_t sample_count_upsampled_all_channels = sample_count_upsampled * audio_thread_data->channel_count;
        assert(sample_count_upsampled_all_channels % 2 == 0);
        int32_t sample_count_final = sample_count_upsampled / M;
        int32_t sample_count_final_all_channels = sample_count_final * audio_thread_data->channel_count;
        assert(sample_count_final_all_channels % 2 == 0);
        // Upsample by zero-stuffing
        memset(upsampled_audio_data, 0, sample_count_upsampled_all_channels * bps);
        for (uint8_t channel = 0; channel < audio_thread_data->channel_count; channel++)
        {
            int32_t channel_offset = channel * bps;
            for (int32_t i = 0; i < sample_count; i++)
            {
                int32_t tmp_src = (i * audio_thread_data->channel_count) + channel_offset;
                int32_t tmp_dst = (i * audio_thread_data->channel_count * L) + channel_offset;
                memcpy(upsampled_audio_data + (i * audio_thread_data->channel_count * L * bps) + channel_offset, audio_buffers[audio_buffer_index] + (i * audio_thread_data->channel_count * bps) + channel_offset, bps);
            }
        }
        // Copy the filter_length samples in the prefetch buffer into the start of the actual buffer to be used for filtering
        memcpy(upsampled_audio_data_proper, prefetch_buffer, filter_length * audio_thread_data->channel_count * bps);
        // Copy the first N - filter_length samples into the actual buffer to be used for filtering at an offset
        memcpy(upsampled_audio_data_proper + (filter_length * audio_thread_data->channel_count * bps), upsampled_audio_data, sample_count_upsampled_all_channels * bps);
        // Copy the last filter_length samples into the prefetch buffer for the next iteration
        memcpy(prefetch_buffer, upsampled_audio_data + (sample_count_upsampled_all_channels * bps) - (filter_length * audio_thread_data->channel_count * bps), filter_length * audio_thread_data->channel_count * bps);
        // Create filter
        int32_t filter_sample_rate = input_rate * L;
        float filter_sample_delta = 1.0f / (float)filter_sample_rate;
        float cutoff_freq = audio_thread_data->sample_rate / 2;
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
        // Normalize filter
        for (int32_t i = 0; i < filter_length; i++)
        {
            filter[i] /= filter_sum;
        }
        // Filter
        for (uint8_t channel = 0; channel < audio_thread_data->channel_count; channel++)
        {
            for (int32_t i = 0; i < sample_count_upsampled; i++)
            {
                float filtered_sample = 0.0f;
                int32_t filter_length_for_sample = min(filter_length, sample_count_upsampled + filter_length - i);
                assert(filter_length_for_sample == filter_length);
                /*int32_t j_offset = L - (i % L);
                for (int32_t j = j_offset; j < filter_length_for_sample; j += L)
                {
                    filtered_sample += (float)((int16_t*)upsampled_audio_data_proper)[current_sample_index + j] * filter[j];
                }*/
                // Don't optimize for now, since we don't know which index our first non-zero-stuffed value is in in our 'proper' upsampled buffer
                for (int32_t j = 0; j < filter_length_for_sample; j++)
                {
                    int32_t tmp_src = (i * audio_thread_data->channel_count) + (j * audio_thread_data->channel_count) + channel;
                    filtered_sample += (float)((int16_t*)upsampled_audio_data_proper)[(i * audio_thread_data->channel_count) + (j * audio_thread_data->channel_count) + channel] * filter[j];
                }
                ((int16_t*)upsampled_audio_data_filtered)[(i * audio_thread_data->channel_count) + channel] = filtered_sample;
            }
        }
        // Decimate
        for (uint8_t channel = 0; channel < audio_thread_data->channel_count; channel++)
        {
            int32_t channel_offset_src = sample_count_upsampled * channel;
            int32_t channel_offset_dst = sample_count_final * channel;
            for (int32_t i = 0; i < sample_count_final; i++)
            {
                int32_t tmp_src = (i * M * audio_thread_data->channel_count) + channel;
                int32_t tmp_dst = (i * audio_thread_data->channel_count) + channel;
                ((int16_t*)upsampled_audio_data_finals[audio_buffer_index])[(i * audio_thread_data->channel_count) + channel] = ((int16_t*)upsampled_audio_data_filtered)[(i * M * audio_thread_data->channel_count) + channel];
            }
        }
        // Send audio data to audio device
        audio_headers[audio_buffer_index].lpData = (LPSTR)upsampled_audio_data_finals[audio_buffer_index];
        audio_headers[audio_buffer_index].dwBufferLength = sample_count_final_all_channels * bps;
        audio_headers[audio_buffer_index].dwBytesRecorded = 0;
        audio_headers[audio_buffer_index].dwUser = NULL;
        audio_headers[audio_buffer_index].dwFlags = 0;
        audio_headers[audio_buffer_index].dwLoops = 0;
#else
        // Send audio data to audio device
        audio_headers[audio_buffer_index].lpData = (LPSTR)audio_buffers[audio_buffer_index];
        audio_headers[audio_buffer_index].dwBufferLength = audio_buffer_data_available_size[audio_buffer_index];
        audio_headers[audio_buffer_index].dwBytesRecorded = 0;
        audio_headers[audio_buffer_index].dwUser = NULL;
        audio_headers[audio_buffer_index].dwFlags = 0;
        audio_headers[audio_buffer_index].dwLoops = 0;
#endif
        MMRESULT res_mmresult = waveOutPrepareHeader(audio_thread_data->audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
        assert(res_mmresult == MMSYSERR_NOERROR);
        res_mmresult = waveOutWrite(audio_thread_data->audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
        assert(res_mmresult == MMSYSERR_NOERROR);
        audio_buffer_index = (audio_buffer_index + 1) % audio_buffer_count;

        // Wait for callback indicating an audio_buffer finished playback
        SyncWaitOnEvent(audio_event, INFINITE, __FILE__, __LINE__);

        // Unprepare header
        res_mmresult = waveOutUnprepareHeader(audio_thread_data->audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
        assert(res_mmresult == MMSYSERR_NOERROR);

        // Update playback buffer
        SyncLockMutex(audio_thread_data->current_playback_buffer_mutex, INFINITE, __FILE__, __LINE__);
        uint8_t audio_buffer_index_next = (audio_buffer_index + 1) % audio_buffer_count;
        memcpy(audio_thread_data->current_playback_buffer, audio_buffers[audio_buffer_index_next], audio_buffer_data_available_size[audio_buffer_index_next]);
        *audio_thread_data->current_playback_buffer_size = audio_buffer_data_available_size[audio_buffer_index_next];
        SyncReleaseMutex(audio_thread_data->current_playback_buffer_mutex, __FILE__, __LINE__);
    }

    return EXIT_SUCCESS;
}

DWORD WINAPI SoundPlayerThreadProc(_In_ LPVOID lpParameter)
{
    // Cast input pointer
    sound_player_shared_data_t* shared_data = (sound_player_shared_data_t*)lpParameter;
    
    // Audio thread
    HANDLE audio_thread = NULL;
    wchar_t audio_thread_name[] = L"bragi_audio_thread";
    audio_thread_shared_data_t audio_thread_data;
    audio_thread_data.sound_player_event = shared_data->next_operation_changed_event;
    audio_thread_data.sound_player_mutex = shared_data->mutex;
    audio_thread_data.sound_player_op = &shared_data->next_operation;
    audio_thread_data.current_playback_buffer_mutex = shared_data->current_playback_buffer_mutex;
    audio_thread_data.current_playback_buffer = shared_data->current_playback_buffer;
    audio_thread_data.current_playback_buffer_size = &shared_data->current_playback_buffer_size;
    
    // Windows audio device data
    HWAVEOUT* windows_audio_device = &shared_data->audio_device;
    WAVEOUTCAPS windows_audio_device_capabilities;
    WAVEFORMATEX windows_audio_device_format;
    WAVEHDR windows_audio_device_wave_header;
    windows_audio_device_wave_header.lpData = NULL;
    windows_audio_device_wave_header.dwFlags = 0;

    // Two playlists to keep track of currently playing playlist, and next playlist to be played
    playlist_t playlist_current, playlist_next;
    PlaylistInit(&playlist_current);
    PlaylistInit(&playlist_next);

    // Two songs to keep track of currently playing song, and next song to be played
    shared_data->song = NULL;

    // Local copies of shared data
    sound_player_operation_e next_operation;

    // Loop
    while (1)
    {
        // Wait for sound player operation to change (can happen from either callback function or the main thread)
        SyncWaitOnEvent(shared_data->next_operation_changed_event, INFINITE, __FILE__, __LINE__);

        // Acquire mutex to alter shared data
        SyncLockMutex(shared_data->mutex, INFINITE, __FILE__, __LINE__);

        // Store local copies of shared data
        next_operation = shared_data->next_operation;

        // Mark sound player operation as busy
        shared_data->next_operation = SOUND_PLAYER_OP_BUSY;

        // Local variables to iteration
        song_t* song_current = shared_data->song;
        song_t* song_next = NULL;
        playlist_error_e playlist_error = PLAYLIST_ERROR_NO;
        song_error_e song_error = SONG_ERROR_NO;
        bool reset_error = false;

        // Handle next operation
        switch (next_operation)
        {
            case SOUND_PLAYER_OP_READY:
            {
                assert(0);
            } break;

            // This means we're loading a new playlist, and will try to load the first song in the playlist.
            // It's possible for this to fail, in which the currently loaded playlist should be kept alive,
            // and no change in behavior/state should be observed.
            case SOUND_PLAYER_OP_PLAY:
            {
                // 1) Load playlist into playlist_next
                playlist_error = PlaylistLoad(shared_data->playlist_next_file_path, &playlist_next);
                switch (playlist_error)
                {
                    case PLAYLIST_ERROR_NO: {} break;

                    case PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE:
                    {
                        sprintf(shared_data->error_message, "Unable to open playlist: %s", shared_data->playlist_next_file_path);
                        shared_data->error_message_changed = true;
                    } break;

                    case PLAYLIST_ERROR_EMPTY:
                    {
                        sprintf(shared_data->error_message, "Playlist file is empty: %s", shared_data->playlist_next_file_path);
                        shared_data->error_message_changed = true;
                    } break;

                    default:
                    {
                        printf("%s:%i Invalid error returned from PlaylistLoad()\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    } break;
                }
                if (playlist_error != PLAYLIST_ERROR_NO)
                {
                    // Loading of playlist was incomplete, but there's no need to free anything in 'playlist_next'
                    PlaylistInit(&playlist_next);
                    break;
                }

                // Potentially shuffle playlist
                if (shared_data->shuffle_state == SOUND_PLAYER_SHUFFLE_RANDOM)
                {
                    PlaylistShuffle(&playlist_next);
                    song_next = &playlist_next.songs_shuffled[playlist_next.current_song_index];
                }
                else // SOUND_PLAYER_SHUFFLE_NO
                {
                    song_next = &playlist_next.songs[playlist_next.current_song_index];
                }

                // 2) Load sound file
                switch (song_next->song_type)
                {
                    case SONG_TYPE_WAV:
                    {
                        song_error = WAVLoadHeader(song_next);
                    } break;

                    case SONG_TYPE_FLAC:
                    {
                        //song_error = FLACLoad(song_next);
                    } break;

                    default:
                    {
                        printf("%s%i: Invalid sound file type %i\n", __FILE__, __LINE__, song_next->song_type);
                        exit(EXIT_FAILURE);
                    }
                }
                switch (song_error)
                {
                    case SONG_ERROR_NO: {} break;

                    case SONG_ERROR_UNABLE_TO_OPEN_FILE:
                    {
                        sprintf(shared_data->error_message, "Unable to open audio file: %s", song_next->song_path_offset);
                        shared_data->error_message_changed = true;
                    } break;

                    case SONG_ERROR_INVALID_FILE:
                    {
                        sprintf(shared_data->error_message, "Not a proper audio file: %s", song_next->song_path_offset);
                        shared_data->error_message_changed = true;
                    } break;

                    default:
                    {
                        printf("%s:%i Invalid error returned\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    } break;
                }
                if (song_error != SONG_ERROR_NO)
                {
                    // Loading of sound file was incomplete, but no need to free anything in 'song_next'.
                    // 'playlist_next' needs to be freed and reinitialized.
                    PlaylistFree(&playlist_next);
                    PlaylistInit(&playlist_next);
                    break;
                }

                // 3) Pick audio device WAVE_MAPPER, and check that it can play the next WAV file's format
                if (AudioDeviceSupportsPlayback(song_next, &windows_audio_device_capabilities) == false)
                {
                    sprintf(shared_data->error_message, "Unsupported audio format:\n\tSample rate: %i\n\tBits per sample: %i", song_next->sample_rate, song_next->bps * 8);
                    shared_data->error_message_changed = true;

                    // Loading of sound file was complete, but playback isn't supported.
                    // 'song_next''s audio data must be freed.
                    // 'playlist_next' must be freed and reinitialized.
                    SongFreeAudioData(song_next);
                    PlaylistFree(&playlist_next);
                    PlaylistInit(&playlist_next);
                    break;
                }
                windows_audio_device_format.wFormatTag = WAVE_FORMAT_PCM;
                windows_audio_device_format.nChannels = song_next->channel_count;
                windows_audio_device_format.nSamplesPerSec = song_next->sample_rate;
                windows_audio_device_format.nAvgBytesPerSec = (song_next->sample_rate * song_next->channel_count * song_next->bps);
                windows_audio_device_format.nBlockAlign = (song_next->channel_count * song_next->bps); // If wFormatTag is WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE, nBlockAlign must be equal to the product of nChannels and wBitsPerSample divided by 8 (bits per byte).
                windows_audio_device_format.wBitsPerSample = song_next->bps * 8;
                windows_audio_device_format.cbSize = 0;

                // 4) If audio device is already open, close it
                if (*windows_audio_device != NULL)
                {
                    AudioClose(*windows_audio_device, audio_thread, audio_headers, audio_buffer_count);
                }

                // 5) Play song
                AudioOpen(windows_audio_device, &windows_audio_device_format, (DWORD_PTR)&waveOutProc, (DWORD_PTR)shared_data);
                AudioPlay(*windows_audio_device, song_next, &AudioThreadProc, &audio_thread_data, audio_thread_name, &audio_thread);

                // Reaching this point means there were no errors
                reset_error = true;

                // Free current song's memory if it exists
                if (song_current != NULL)
                {
                    SongFreeAudioData(song_current);
                }
                // Update current song
                shared_data->song = song_next;
                // Free current playlist's memory
                if (playlist_current.songs != NULL)
                {
                    PlaylistFree(&playlist_current);
                }
                // Update current playlist
                playlist_current = playlist_next;
                strcpy(shared_data->playlist_current_file_path, shared_data->playlist_next_file_path);
                shared_data->playlist_current_changed = true;
                PlaylistInit(&playlist_next);
            } break;

            // This means that the current playlist will remain the current playlist.
            // The next song in the playlist will be played.
            case SOUND_PLAYER_OP_NEXT:
            {
                assert(playlist_current.songs != NULL);

                // 1) Select next sound file to play
                if ((shared_data->loop_state == SOUND_PLAYER_LOOP_SINGLE) ||
                    ((shared_data->loop_state == SOUND_PLAYER_LOOP_PLAYLIST) && (playlist_current.song_count == 1)))
                {
                    // We have all the info, so just seek to the start of the audio data and continue as normal
                    fseek(song_current->file, song_current->file_size - song_current->audio_data_size, SEEK_SET);
                    break;
                }
                else
                {
                    playlist_current.current_song_index++;
                    if (playlist_current.current_song_index >= playlist_current.song_count)
                    {
                        if (shared_data->loop_state == SOUND_PLAYER_LOOP_NO)
                        {
                            strcpy(shared_data->error_message, "End of playlist reached");
                            shared_data->error_message_changed = true;
                            playlist_current.current_song_index--;
                            break;
                        }
                        else // SOUND_PLAYER_LOOP_PLAYLIST
                        {
                            playlist_current.current_song_index = 0;
                        }
                    }
                }

                // 2) Pick sound file
                if (shared_data->shuffle_state == SOUND_PLAYER_SHUFFLE_RANDOM)
                {
                    song_next = &playlist_current.songs_shuffled[playlist_current.current_song_index];
                }
                else // SOUND_PLAYER_SHUFFLE_NO
                {
                    song_next = &playlist_current.songs[playlist_current.current_song_index];
                }

                // 3) Load sound file
                switch (song_next->song_type)
                {
                    case SONG_TYPE_WAV:
                    {
                        song_error = WAVLoadHeader(song_next);
                    } break;

                    case SONG_TYPE_FLAC:
                    {
                        assert(0);
                        //song_error = FLACLoad(song_next);
                    } break;

                    default:
                    {
                        printf("%s%i: Invalid sound type %i\n", __FILE__, __LINE__, song_next->song_type);
                        exit(EXIT_FAILURE);
                    }
                }
                switch (song_error)
                {
                    case SONG_ERROR_NO: {} break;

                    case SONG_ERROR_UNABLE_TO_OPEN_FILE:
                    {
                        sprintf(shared_data->error_message, "Unable to open audio file: %s", song_next->song_path_offset);
                        shared_data->error_message_changed = true;
                    } break;

                    case SONG_ERROR_INVALID_FILE:
                    {
                        sprintf(shared_data->error_message, "Not a proper audio file: %s", song_next->song_path_offset);
                        shared_data->error_message_changed = true;
                    } break;

                    default:
                    {
                        printf("%s:%i Invalid error returned\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    } break;
                }
                if (song_error != SONG_ERROR_NO)
                {
                    // Loading of sound file was incomplete.
                    // No need to free audio data.
                    break;
                }

                // 4) Pick audio device WAVE_MAPPER, and check that it can play the next WAV file's format
                if (AudioDeviceSupportsPlayback(song_next, &windows_audio_device_capabilities) == false)
                {
                    sprintf(shared_data->error_message, "Unsupported audio format:\n\tSample rate: %i\n\tBits per sample: %i", song_next->sample_rate, song_next->bps * 8);
                    shared_data->error_message_changed = true;

                    // Loading of WAV file was complete, but playback isn't supported.
                    // 'song_next''s audio data must be freed.
                    SongFreeAudioData(song_next);
                    break;
                }
                windows_audio_device_format.wFormatTag = WAVE_FORMAT_PCM;
                windows_audio_device_format.nChannels = song_next->channel_count;
                windows_audio_device_format.nSamplesPerSec = song_next->sample_rate;
                windows_audio_device_format.nAvgBytesPerSec = song_next->sample_rate * song_next->channel_count * song_next->bps;
                windows_audio_device_format.nBlockAlign = song_next->channel_count * song_next->bps; // If wFormatTag is WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE, nBlockAlign must be equal to the product of nChannels and wBitsPerSample divided by 8 (bits per byte).
                windows_audio_device_format.wBitsPerSample = song_next->bps * 8;
                windows_audio_device_format.cbSize = 0;

                // 5) Re-open audio device with new settings
                // If the previous file tried played didn't play, the audio device might not be open
                // It is not open if there is already a song playing when the previous file was tried played
                if (*windows_audio_device != NULL)
                {
                    // Will invoke callback
                    AudioClose(*windows_audio_device, audio_thread, audio_headers, audio_buffer_count);
                }

                // 6) Play next sound file
                AudioOpen(windows_audio_device, &windows_audio_device_format, (DWORD_PTR)&waveOutProc, (DWORD_PTR)shared_data);
                AudioPlay(*windows_audio_device, song_next, AudioThreadProc, &audio_thread_data, audio_thread_name, &audio_thread);

                // Reaching this point means there were no errors
                reset_error = true;

                // Free current song's memory if one is loaded
                assert(song_current->file != NULL);
                SongFreeAudioData(song_current);
                // Update current song
                shared_data->song = song_next;
            } break;

            case SOUND_PLAYER_OP_PREVIOUS:
            {
                // TODO (Daniel): when OP_NEXT handling is stable, copy-paste it and change the direction of the next song
                assert(0);
            } break;

            case SOUND_PLAYER_OP_PAUSE:
            {
                AudioPause(*windows_audio_device);

                // Reaching this point means there were no errors
                reset_error = true;
            } break;

            case SOUND_PLAYER_OP_RESUME:
            {
                AudioResume(*windows_audio_device);

                // Reaching this point means there were no errors
                reset_error = true;
            } break;

            case SOUND_PLAYER_OP_SHUFFLE:
            {
                // Check that a playlist is currently loaded before trying to shuffle it
                if (playlist_current.songs != NULL)
                {
                    PlaylistShuffle(&playlist_current);

                    // Reaching this point means there were no errors
                    reset_error = true;
                }
            } break;

            default:
            {
                assert(0);
            }
        }

        // Update shared data
        shared_data->next_operation = SOUND_PLAYER_OP_READY;
        if (reset_error == true)
        {
            shared_data->error_message[0] = '\0';
            shared_data->error_message_changed = true;
        }

        // Release mutex
        SyncReleaseMutex(shared_data->mutex, __FILE__, __LINE__);
    }

    return EXIT_SUCCESS;
}