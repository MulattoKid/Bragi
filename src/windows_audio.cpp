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

#include "windows_audio.h"
#include "windows_synchronization.h"
#include "windows_thread.h"

#include <assert.h>
#include <stdio.h>

bool AudioDeviceSupportsPlayback(song_t* song, LPWAVEOUTCAPS windows_audio_device_capabilities)
{
    assert(song != NULL);
    assert(windows_audio_device_capabilities != NULL);

    MMRESULT res = waveOutGetDevCaps(WAVE_MAPPER, windows_audio_device_capabilities, sizeof(WAVEOUTCAPS));
    assert(res == MMSYSERR_NOERROR);
    // TODO (Daniel): currently only supportiong stereo playback
    assert(windows_audio_device_capabilities->wChannels == song->channel_count);
    if ((song->sample_rate == 11025) &&
        (song->bps == 1))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_1S08)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 11025) &&
             (song->bps == 2))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_1S16)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 22050) &&
             (song->bps == 1))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_2S08)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 22050) &&
             (song->bps == 2))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_2S16)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 44100) &&
             (song->bps == 1))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_44S08)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 44100) &&
             (song->bps == 2))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_44S16)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 48000) &&
             (song->bps == 1))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_48S08)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ((song->sample_rate == 48000) &&
             (song->bps == 2))
    {
        if (windows_audio_device_capabilities->dwFormats & WAVE_FORMAT_48S16)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void AudioOpen(LPHWAVEOUT device, LPCWAVEFORMATEX device_format, DWORD_PTR callback, DWORD_PTR shared_data)
{
    assert(device != NULL);

    MMRESULT res_mmresult = waveOutOpen(device, WAVE_MAPPER, device_format, callback, shared_data, CALLBACK_FUNCTION);
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioPlay(HWAVEOUT device, song_t* song, LPTHREAD_START_ROUTINE thread_function, audio_thread_shared_data_t* audio_thread_data, wchar_t* thread_name, HANDLE* thread_handle)
{
    assert(device != NULL);
    assert(song != NULL);
    assert(thread_function != NULL);
    assert(audio_thread_data != NULL);
    assert(thread_name != NULL);
    assert(thread_handle != NULL);

    audio_thread_data->audio_device = device;
    audio_thread_data->file = song->file;
    audio_thread_data->file_size = song->file_size;
    audio_thread_data->sample_rate = song->sample_rate;
    audio_thread_data->bps = song->bps;
    audio_thread_data->channel_count = song->channel_count;
    ThreadCreate(thread_function, audio_thread_data, thread_name, thread_handle);
}

void AudioPause(HWAVEOUT device)
{
    assert(device != NULL);

    MMRESULT res_mmresult = waveOutPause(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioResume(HWAVEOUT device)
{
    assert(device != NULL);

    MMRESULT res_mmresult = waveOutRestart(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioGetPlaybackPosition(HWAVEOUT device, LPMMTIME playback_position)
{
    assert(device != NULL);
    assert(playback_position != NULL);

    MMRESULT res_mmresult = waveOutGetPosition(device, playback_position, sizeof(MMTIME));
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioClose(HWAVEOUT device, HANDLE thread_handle, LPWAVEHDR headers, uint8_t header_count)
{
    assert(device != NULL);
    assert(thread_handle != NULL);

    // Kill audio loader thread
    ThreadDestroy(thread_handle, EXIT_SUCCESS);

    // Reset audio device
    MMRESULT res_mmresult = waveOutReset(device);
    assert(res_mmresult == MMSYSERR_NOERROR);

    // Wait for all headers to indicate the header is DONE
    for (uint32_t i = 0; i < header_count; i++)
    {
        while (((headers[i].dwFlags & WHDR_DONE) != WHDR_DONE)) {}
    }

    // Close audio device
    res_mmresult = waveOutClose(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
}