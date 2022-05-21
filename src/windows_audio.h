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

#ifndef WINDOWS_AUDIO_H
#define WINDOWS_AUDIO_H

#include "song.h"
#include "sound_player.h"

#include <windows.h>

bool AudioDeviceSupportsPlayback(uint32_t sample_rate, uint8_t bps, uint8_t channel_count);
void AudioOpen(LPHWAVEOUT device, LPCWAVEFORMATEX device_format, DWORD_PTR callback, DWORD_PTR shared_data);
void AudioPause(HWAVEOUT device);
void AudioResume(HWAVEOUT device);
void AudioGetPlaybackPosition(HWAVEOUT device, LPMMTIME playback_position);
void AudioClose(HWAVEOUT device, LPWAVEHDR headers, uint8_t header_count);

#endif