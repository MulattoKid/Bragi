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

#include "windows_synchronization.h"
#include "windows_audio.h"

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
        (song->bits_per_sample == 8))
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
             (song->bits_per_sample == 16))
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
             (song->bits_per_sample == 8))
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
             (song->bits_per_sample == 16))
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
             (song->bits_per_sample == 8))
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
             (song->bits_per_sample == 16))
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
             (song->bits_per_sample == 8))
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
             (song->bits_per_sample == 16))
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

/**
 * This function will
 *  1) Prepare the header
 *  2) Open the audio device
*/
void AudioPlay(HWAVEOUT device, song_t* song, LPWAVEHDR header)
{
    assert(device != NULL);
    assert(song != NULL);
    assert(header != NULL);

    header->lpData = (LPSTR)song->audio_data;
    header->dwBufferLength = song->audio_data_size;
    header->dwBytesRecorded = 0;
    header->dwUser = NULL;
    header->dwFlags = 0;
    header->dwLoops = 0;
    MMRESULT res_mmresult = waveOutPrepareHeader(device, header, sizeof(WAVEHDR));
    assert(res_mmresult == MMSYSERR_NOERROR);
    res_mmresult = waveOutWrite(device, header, sizeof(WAVEHDR));
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioPause(HWAVEOUT device)
{
    assert(device);

    MMRESULT res_mmresult = waveOutPause(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
}

void AudioResume(HWAVEOUT device)
{
    assert(device);

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

/**
 * This function will
 *  1) Stop playback
 *  2) Unprepare the header
 *  3) Close the audio device
 * Doing the first two steps is unnecessary if the device isn't playing anything,
 * but shouldn't be an issue.
*/
void AudioClose(HANDLE mutex, LPHWAVEOUT device, LPWAVEHDR header)
{
    assert(device != NULL);
    assert(header != NULL);
    
    // Set the device to be NULL before releasing the mutex, so that other parts that might acquire the mutex
    // will act as if the device is already closed (i.e. not use it)
    HWAVEOUT audio_device_to_be_closed = *device;
    *device = NULL;

    // Release mutex before stopping playback
    // Because we've set the shared data's audio device handle to be NULL before releasing the mutex,
    // other threads that acquire the mutex and look at it will think it's not available, and act accordingly
    // (even though the device is still playing and still open)
    SyncReleaseMutex(mutex, __FILE__, __LINE__);

    // Stop playback
    MMRESULT res_mmresult = waveOutReset(audio_device_to_be_closed);
    assert(res_mmresult == MMSYSERR_NOERROR);
    assert(header->lpData != NULL);

    // Busy-wait for the callback to have finished handling the WOM_DONE message
    // TODO (Daniel): should there be a Sleep(1) inside the while-loop?
    while ((header->dwFlags & WHDR_DONE) != WHDR_DONE) {}
    SyncLockMutex(mutex, INFINITE, __FILE__, __LINE__);

    // Unprepare header
    res_mmresult = waveOutUnprepareHeader(audio_device_to_be_closed, header, sizeof(WAVEHDR));
    assert(res_mmresult == MMSYSERR_NOERROR);

    // Close device
    res_mmresult = waveOutClose(audio_device_to_be_closed);
    assert(res_mmresult == MMSYSERR_NOERROR);
}