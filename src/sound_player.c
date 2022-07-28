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

#include "audio.h"
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
#define audio_buffer_count 3
#define audio_buffer_size 8192
static WAVEHDR audio_headers[audio_buffer_count];
static byte_t audio_buffers[audio_buffer_count][audio_buffer_size];
static uint32_t audio_buffer_data_available_size[audio_buffer_count];
static uint8_t audio_buffer_index = 0;

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    // Cast input pointer
    callback_data_t* callback_data = (callback_data_t*)dwInstance;

    // Handle message
    switch (uMsg)
    {
        case WOM_OPEN: {} break;
        
        case WOM_CLOSE: {} break;

        case WOM_DONE:
        {
            InterlockedIncrement((volatile LONG*)&callback_data->callback_count_atomic);
            SyncSetEvent(callback_data->event, __FILE__, __LINE__);
        } break;

        default:
        {
            assert(0);
        } break;
    }
}

static uint8_t filter_length = 64;
static float* filter = NULL;
static byte_t* prefetch_buffer = NULL;
static byte_t* upsampled_audio_data = NULL;
static byte_t* upsampled_audio_data_with_prefetch_buffer = NULL;
static byte_t* upsampled_audio_data_filtered = NULL;
static byte_t* upsampled_audio_data_finals[audio_buffer_count];
static float slow_down_factor = 1.0f;//0.8f;
DWORD WINAPI SoundPlayerThreadProc(_In_ LPVOID lpParameter)
{
    // Cast input pointer
    sound_player_shared_data_t* shared_data = (sound_player_shared_data_t*)lpParameter;
    
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

    // Data about current song for sample-rate conversion
    uint32_t input_rate;
    float resampling_factor;
    uint32_t output_rate;
    uint32_t gcd;
    uint32_t L; // Upsampling factor
    uint32_t M; // Decimation factor
    uint32_t bps;
    uint32_t channel_count;
    uint32_t bps_all_channels;
    uint32_t max_sample_count_in_audio_buffer;
    uint32_t max_sample_count_in_audio_buffer_all_channels;
    uint32_t max_sample_count_upsampled_all_channels;
    uint32_t max_sample_count_decimated_all_channels;

    // Playback data about current song
    playback_data_t playback_data;

    // Callback data
    callback_data_t callback_data;
    callback_data.event = shared_data->event;
    callback_data.callback_count_atomic = 0;
    sound_player_operation_e sound_player_next_operation = SOUND_PLAYER_OP_READY;

    // Loop
    while (1)
    {
        // Wait for the event to be signaled by either the UI thread or the callback
        SyncWaitOnEvent(shared_data->event, INFINITE, __FILE__, __LINE__);
        
        // Acquire mutex to access shared data
        SyncLockMutex(shared_data->mutex, INFINITE, __FILE__, __LINE__);

        // Track whether later handling should be overruled
        uint8_t sound_player_operation_overruled = 0;
        uint8_t callback_count_overruled = 0;

        sound_player_operation_e ui_next_operation = shared_data->ui_next_operation;
        // Check if we're to handle a command from the UI thread
        if (ui_next_operation != SOUND_PLAYER_OP_READY)
        {
            // Regardless of the result of handling the UI thread's next operation it will be reset
            shared_data->ui_next_operation = SOUND_PLAYER_OP_READY;

            // Local variables
            song_t* song_current = shared_data->song;
            song_t* song_next = NULL;
            playlist_error_e playlist_error = PLAYLIST_ERROR_NO;
            song_error_e song_error = SONG_ERROR_NO;
            uint8_t operation_success = 0;
            uint8_t load_initial_chunks = 0;

            // Handle next operation
            switch (ui_next_operation)
            {
                case SOUND_PLAYER_OP_READY:
                {
                    assert(0); // Shouldn't happen
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
                            shared_data->error_message_changed = 1;
                        } break;

                        case PLAYLIST_ERROR_EMPTY:
                        {
                            sprintf(shared_data->error_message, "Playlist file is empty: %s", shared_data->playlist_next_file_path);
                            shared_data->error_message_changed = 1;
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
                            assert(0);
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
                            shared_data->error_message_changed = 1;
                        } break;

                        case SONG_ERROR_INVALID_FILE:
                        {
                            sprintf(shared_data->error_message, "Not a proper audio file: %s", song_next->song_path_offset);
                            shared_data->error_message_changed = 1;
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
                    if (AudioDeviceSupportsPlayback(song_next->sample_rate, song_next->bps, song_next->channel_count) == 0)
                    {
                        sprintf(shared_data->error_message, "Unsupported audio format:\n\tSample rate: %i\n\tBits per sample: %i", song_next->sample_rate, song_next->bps * 8);
                        shared_data->error_message_changed = 1;

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
                    windows_audio_device_format.nBlockAlign = (song_next->channel_count * song_next->bps);
                    windows_audio_device_format.wBitsPerSample = song_next->bps * 8;
                    windows_audio_device_format.cbSize = 0;

                    // 4) If audio device is already open, close it
                    if (*windows_audio_device != NULL)
                    {
                        AudioClose(*windows_audio_device, audio_headers, audio_buffer_count);
                    }

                    // 5) Open audio device with current settings
                    AudioOpen(windows_audio_device, &windows_audio_device_format, (DWORD_PTR)&waveOutProc, (DWORD_PTR)&callback_data);

                    // Reaching this point means there were no errors
                    operation_success = 1;
                    load_initial_chunks = 1;
                    sound_player_operation_overruled = 1;
                    callback_count_overruled = 1;
                    SyncResetEvent(shared_data->event, __FILE__, __LINE__);

                    // Update current song
                    if (song_current != NULL)
                    {
                        SongFreeAudioData(song_current);
                    }
                    shared_data->song = song_next;
                    // Update current playlist
                    if (playlist_current.songs != NULL)
                    {
                        PlaylistFree(&playlist_current);
                    }
                    playlist_current = playlist_next;
                    strcpy(shared_data->playlist_current_file_path, shared_data->playlist_next_file_path);
                    shared_data->playlist_current_changed = 1;
                    PlaylistInit(&playlist_next);
                } break;

                // The next song in the playlist will be played
                case SOUND_PLAYER_OP_NEXT:
                {
                    assert(playlist_current.songs != NULL);

                    // 1) Select next sound file to play
                    // TODO: this case could be optimized
                    /*if ((shared_data->loop_state == SOUND_PLAYER_LOOP_SINGLE) ||
                        ((shared_data->loop_state == SOUND_PLAYER_LOOP_PLAYLIST) && (playlist_current.song_count == 1)))
                    {}*/
                    playlist_current.current_song_index++;
                    if (playlist_current.current_song_index >= playlist_current.song_count)
                    {
                        if (shared_data->loop_state == SOUND_PLAYER_LOOP_NO)
                        {
                            strcpy(shared_data->error_message, "End of playlist reached");
                            shared_data->error_message_changed = 1;
                            playlist_current.current_song_index--;
                            break;
                        }
                        else // SOUND_PLAYER_LOOP_PLAYLIST
                        {
                            playlist_current.current_song_index = 0;
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
                            shared_data->error_message_changed = 1;
                        } break;

                        case SONG_ERROR_INVALID_FILE:
                        {
                            sprintf(shared_data->error_message, "Not a proper audio file: %s", song_next->song_path_offset);
                            shared_data->error_message_changed = 1;
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
                    if (AudioDeviceSupportsPlayback(song_next->sample_rate, song_next->bps, song_next->channel_count) == 0)
                    {
                        sprintf(shared_data->error_message, "Unsupported audio format:\n\tSample rate: %i\n\tBits per sample: %i", song_next->sample_rate, song_next->bps * 8);
                        shared_data->error_message_changed = 1;

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

                    // 6) Play next sound file
                    assert(windows_audio_device != NULL);
                    AudioClose(*windows_audio_device, audio_headers, audio_buffer_count);
                    AudioOpen(windows_audio_device, &windows_audio_device_format, (DWORD_PTR)&waveOutProc, (DWORD_PTR)&callback_data);

                    // Reaching this point means there were no errors
                    operation_success = 1;
                    load_initial_chunks = 1;
                    sound_player_operation_overruled = 1;
                    callback_count_overruled = 1;

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
                    operation_success = 1;
                    sound_player_operation_overruled = 1;
                } break;

                case SOUND_PLAYER_OP_RESUME:
                {
                    AudioResume(*windows_audio_device);

                    // Reaching this point means there were no errors
                    operation_success = 1;
                    sound_player_operation_overruled = 1;
                } break;

                case SOUND_PLAYER_OP_SHUFFLE:
                {
                    // Check that a playlist is currently loaded before trying to shuffle it
                    if (playlist_current.songs != NULL)
                    {
                        PlaylistShuffle(&playlist_current);

                        // Reaching this point means there were no errors
                        operation_success = 1;
                    }
                } break;

                default:
                {
                    assert(0);
                }
            }

            // If the operation was handled successfully, it means the previous error can be cleared
            if (operation_success == 1)
            {
                shared_data->error_message[0] = '\0';
                shared_data->error_message_changed = 1;
            }

            // If the operation was handled successfully and the operation was either PLAY, NEXT or PREVIOUS
            // we want to
            //  1) reset the callback counter, so that we only start loading in new chunks when the initial chunks of the new songs start to finish playback
            //  2) reset the event, so that if the callback has signaled the event while we were handling the operation, we ignore that signal as we're loading
            //     in new initial chunks, and reset the callback counter
            if ((operation_success == 1) &&
                ((ui_next_operation == SOUND_PLAYER_OP_PLAY) || (ui_next_operation == SOUND_PLAYER_OP_NEXT) || (ui_next_operation == SOUND_PLAYER_OP_PREVIOUS)))
            {
                InterlockedExchange((volatile LONG*)&callback_data.callback_count_atomic, 0);
                SyncResetEvent(shared_data->event, __FILE__, __LINE__);
            }

            // If this is set we've loaded a new song (either through PLAY, NEXT or PREVIOUS), and we need to load its initial chunks
            if (load_initial_chunks == 1)
            {
                // Compute info about current song for sample-rate conversion
                input_rate = shared_data->song->sample_rate;
                resampling_factor = (input_rate + (input_rate - (slow_down_factor * input_rate))) / input_rate;
                output_rate = input_rate * resampling_factor;
                gcd = FindGreatestCommonDivisor(input_rate, output_rate);
                L = output_rate / gcd; // Upsampling factor
                M = input_rate / gcd; // Decimation factor
                bps = shared_data->song->bps;
                channel_count = shared_data->song->channel_count;
                bps_all_channels = channel_count * bps;
                max_sample_count_in_audio_buffer = audio_buffer_size / bps_all_channels; // Ensure it fits all samples for a channel
                max_sample_count_in_audio_buffer_all_channels = max_sample_count_in_audio_buffer * channel_count; // Scale up to get number of individual samples
                max_sample_count_upsampled_all_channels = max_sample_count_in_audio_buffer_all_channels * L;
                max_sample_count_decimated_all_channels = (max_sample_count_upsampled_all_channels / M);
                max_sample_count_decimated_all_channels -= (max_sample_count_decimated_all_channels % channel_count); // Ensure we can fit entire channels

                // Set playback data
                playback_data.audio_device = shared_data->audio_device;
                playback_data.file = shared_data->song->file;
                playback_data.file_size = shared_data->song->file_size;
                playback_data.sample_rate = shared_data->song->sample_rate;
                playback_data.channel_count = shared_data->song->channel_count;
                playback_data.bps = shared_data->song->bps;

                // Check if any buffers already exists, and if so, free them
                if (filter != NULL)
                {
                    free(filter);
                }
                if (prefetch_buffer != NULL)
                {
                    free(prefetch_buffer);
                }
                if (upsampled_audio_data != NULL)
                {
                    free(upsampled_audio_data);
                }
                if (upsampled_audio_data_with_prefetch_buffer != NULL)
                {
                    free(upsampled_audio_data_with_prefetch_buffer);
                }
                if (upsampled_audio_data_filtered != NULL)
                {
                    free(upsampled_audio_data_filtered);
                }
                for (uint8_t i = 0; i < audio_buffer_count; i++)
                {
                    if (upsampled_audio_data_finals[i] != NULL)
                    {
                        free(upsampled_audio_data_finals[i]);
                    }
                }

                // (Re)Allocate buffers
                filter = (float*)malloc(filter_length * sizeof(float));
                prefetch_buffer = (byte_t*)malloc(filter_length * channel_count * bps);
                upsampled_audio_data = (byte_t*)malloc(max_sample_count_upsampled_all_channels * bps);
                upsampled_audio_data_with_prefetch_buffer = (byte_t*)malloc((max_sample_count_upsampled_all_channels + (filter_length * channel_count)) * bps);
                upsampled_audio_data_filtered = (byte_t*)malloc(max_sample_count_upsampled_all_channels * bps);
                for (uint8_t i = 0; i < audio_buffer_count; i++)
                {
                    upsampled_audio_data_finals[i] = (byte_t*)malloc(max_sample_count_decimated_all_channels * bps);
                }

                // Compute filter
                LowPassFilterCreate(input_rate, L, filter_length, filter, FILTER_TYPE_SINC, WINDOW_TYPE_HAMMING);

                // Preload first N-1 audio_buffers
                audio_buffer_index = 0;
                for (uint32_t i = 0; i < audio_buffer_count - 1; i++)
                {
                    // Load audio data
                    audio_buffer_data_available_size[audio_buffer_index] = WAVLoadData(&playback_data, audio_buffer_size, audio_buffers[audio_buffer_index]);

                    // Ensure there's audio data
                    if (audio_buffer_data_available_size[audio_buffer_index] == 0)
                    {
                        assert(0); // Need to handle this
                    }

                    if (slow_down_factor < 1.0f)
                    {
                        // Perform sample-rate conversion
                        uint32_t sample_count_all_channels = audio_buffer_data_available_size[audio_buffer_index] / bps;
                        uint32_t sample_count_output_all_channels = SampleRateConvert(input_rate, output_rate, L, M, slow_down_factor, sample_count_all_channels, bps, channel_count, audio_buffers[audio_buffer_index], upsampled_audio_data, prefetch_buffer, filter_length, filter, upsampled_audio_data_with_prefetch_buffer, upsampled_audio_data_filtered, upsampled_audio_data_finals[audio_buffer_index]);
                        audio_headers[audio_buffer_index].lpData = (LPSTR)upsampled_audio_data_finals[audio_buffer_index];
                        audio_headers[audio_buffer_index].dwBufferLength = sample_count_output_all_channels * bps;
                        audio_headers[audio_buffer_index].dwBytesRecorded = 0;
                        audio_headers[audio_buffer_index].dwUser = NULL;
                        audio_headers[audio_buffer_index].dwFlags = 0;
                        audio_headers[audio_buffer_index].dwLoops = 0;
                    }
                    else
                    {
                        audio_headers[audio_buffer_index].lpData = (LPSTR)audio_buffers[audio_buffer_index];
                        audio_headers[audio_buffer_index].dwBufferLength = audio_buffer_data_available_size[audio_buffer_index];
                        audio_headers[audio_buffer_index].dwBytesRecorded = 0;
                        audio_headers[audio_buffer_index].dwUser = NULL;
                        audio_headers[audio_buffer_index].dwFlags = 0;
                        audio_headers[audio_buffer_index].dwLoops = 0;
                    }
                    // Send audio data to audio device
                    MMRESULT res_mmresult = waveOutPrepareHeader(playback_data.audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
                    assert(res_mmresult == MMSYSERR_NOERROR);
                    res_mmresult = waveOutWrite(playback_data.audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
                    assert(res_mmresult == MMSYSERR_NOERROR);
                    audio_buffer_index = (audio_buffer_index + 1) % audio_buffer_count;
                }
            }
        }

        // Check if we're to handle a command from the sound player
        if (sound_player_operation_overruled == 1)
        {
            sound_player_next_operation = SOUND_PLAYER_OP_READY;
        }
        else
        {
            if (sound_player_next_operation != SOUND_PLAYER_OP_READY)
            {
                assert(sound_player_next_operation == SOUND_PLAYER_OP_NEXT);

                shared_data->ui_next_operation = sound_player_next_operation;
                sound_player_next_operation = SOUND_PLAYER_OP_READY;
                callback_count_overruled = 1;
                SyncSetEvent(shared_data->event, __FILE__, __LINE__);
            }
        }

        // Release mutex
        SyncReleaseMutex(shared_data->mutex, __FILE__, __LINE__);

        // Check if we're to handle the callback having been invoked
        int32_t callback_count = callback_data.callback_count_atomic;
        if ((callback_count_overruled == 0) &&
            (callback_count > 0))
        {
            // Load next chunk of audio file
            audio_buffer_data_available_size[audio_buffer_index] = WAVLoadData(&playback_data, audio_buffer_size, audio_buffers[audio_buffer_index]);

            // No more data to play back
            if (audio_buffer_data_available_size[audio_buffer_index] == 0)
            {
                sound_player_next_operation = SOUND_PLAYER_OP_NEXT;
                SyncSetEvent(shared_data->event, __FILE__, __LINE__);
            }

            // Check if performing sample-rate conversion is necessary
            assert(slow_down_factor <= 1.0f);
            if (slow_down_factor < 1.0f)
            {
                // Perform sample-rate conversion
                uint32_t sample_count_all_channels = audio_buffer_data_available_size[audio_buffer_index] / bps;
                uint32_t sample_count_output_all_channels = SampleRateConvert(input_rate, output_rate, L, M, slow_down_factor, sample_count_all_channels, bps, channel_count, audio_buffers[audio_buffer_index], upsampled_audio_data, prefetch_buffer, filter_length, filter, upsampled_audio_data_with_prefetch_buffer, upsampled_audio_data_filtered, upsampled_audio_data_finals[audio_buffer_index]);

                // Send audio data to audio device
                audio_headers[audio_buffer_index].lpData = (LPSTR)upsampled_audio_data_finals[audio_buffer_index];
                audio_headers[audio_buffer_index].dwBufferLength = sample_count_output_all_channels * bps;
                audio_headers[audio_buffer_index].dwBytesRecorded = 0;
                audio_headers[audio_buffer_index].dwUser = NULL;
                audio_headers[audio_buffer_index].dwFlags = 0;
                audio_headers[audio_buffer_index].dwLoops = 0;
            }
            else
            {
                // Send audio data to audio device
                audio_headers[audio_buffer_index].lpData = (LPSTR)audio_buffers[audio_buffer_index];
                audio_headers[audio_buffer_index].dwBufferLength = audio_buffer_data_available_size[audio_buffer_index];
                audio_headers[audio_buffer_index].dwBytesRecorded = 0;
                audio_headers[audio_buffer_index].dwUser = NULL;
                audio_headers[audio_buffer_index].dwFlags = 0;
                audio_headers[audio_buffer_index].dwLoops = 0;
            }
            MMRESULT res_mmresult = waveOutPrepareHeader(playback_data.audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
            assert(res_mmresult == MMSYSERR_NOERROR);
            res_mmresult = waveOutWrite(playback_data.audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
            assert(res_mmresult == MMSYSERR_NOERROR);
            audio_buffer_index = (audio_buffer_index + 1) % audio_buffer_count;

            // Unprepare header
            res_mmresult = waveOutUnprepareHeader(playback_data.audio_device, &audio_headers[audio_buffer_index], sizeof(WAVEHDR));
            assert(res_mmresult == MMSYSERR_NOERROR);

            // Update playback buffer
            SyncLockMutex(shared_data->current_playback_buffer_mutex, INFINITE, __FILE__, __LINE__);
            uint8_t audio_buffer_index_next = (audio_buffer_index + 1) % audio_buffer_count;
            memcpy(shared_data->current_playback_buffer, audio_buffers[audio_buffer_index_next], audio_buffer_data_available_size[audio_buffer_index_next]);
            shared_data->current_playback_buffer_size = audio_buffer_data_available_size[audio_buffer_index_next];
            SyncReleaseMutex(shared_data->current_playback_buffer_mutex, __FILE__, __LINE__);

            // Decrement atomic counter
            InterlockedDecrement((volatile LONG*)&callback_data.callback_count_atomic);
        }
    }

    return EXIT_SUCCESS;
}
