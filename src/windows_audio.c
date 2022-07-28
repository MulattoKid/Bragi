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

// https://docs.microsoft.com/en-us/windows/win32/multimedia/determining-nonstandard-format-support
uint8_t AudioDeviceSupportsPlayback(uint32_t sample_rate, uint8_t bps, uint8_t channel_count)
{
    WAVEFORMATEX format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = channel_count;
    format.nSamplesPerSec = sample_rate;
    format.nAvgBytesPerSec = channel_count * sample_rate * bps;
    format.nBlockAlign = channel_count * bps;
    format.wBitsPerSample = bps * 8;
    format.cbSize = 0; // Ignored so long as wFormatTag == WAVE_FORMAT_PCM or WAVE_FORMAT_IEEE_FLOAT
    MMRESULT res = waveOutOpen(NULL, WAVE_MAPPER, &format, NULL, NULL, WAVE_FORMAT_QUERY);
    if (res != MMSYSERR_NOERROR)
    {
        return 0;
    }
    return 1;
}

void AudioOpen(LPHWAVEOUT device, LPCWAVEFORMATEX device_format, DWORD_PTR callback, DWORD_PTR shared_data)
{
    assert(device != NULL);

    MMRESULT res_mmresult = waveOutOpen(device, WAVE_MAPPER, device_format, callback, shared_data, CALLBACK_FUNCTION);
    assert(res_mmresult == MMSYSERR_NOERROR);
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

void AudioClose(HWAVEOUT device, LPWAVEHDR headers, uint8_t header_count)
{
    assert(device != NULL);
    assert(headers != NULL);
    assert(header_count > 0);

    MMRESULT res_mmresult;

    // Reset audio device
    res_mmresult = waveOutReset(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
    
    // Wait for all headers to indicate the header is DONE
    for (uint32_t i = 0; i < header_count; i++)
    {
        // TODO: remove - isn't really neccessary
        // https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutreset
        //    All pending playback buffers are marked as done (WHDR_DONE) and returned to the application.
        while (1)
        {
            if (((headers[i].dwFlags & WHDR_DONE) == WHDR_DONE) ||
                (headers[i].dwFlags == 0))
            {
                break;
            }
        }

        // Unprepare any prepared headers
        if ((headers[i].dwFlags & WHDR_PREPARED) == WHDR_PREPARED)
        {
            res_mmresult = waveOutUnprepareHeader(device, &headers[i], sizeof(WAVEHDR));
            assert(res_mmresult == MMSYSERR_NOERROR);
        }
    }

    // Close audio device
    res_mmresult = waveOutClose(device);
    assert(res_mmresult == MMSYSERR_NOERROR);
}