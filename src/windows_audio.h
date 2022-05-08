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

bool AudioDeviceSupportsPlayback(song_t* song, LPWAVEOUTCAPS windows_audio_device_capabilities);
void AudioOpen(LPHWAVEOUT device, LPCWAVEFORMATEX device_format, DWORD_PTR callback, DWORD_PTR shared_data);
void AudioPlay(HWAVEOUT device, song_t* song, LPTHREAD_START_ROUTINE thread_function, audio_thread_shared_data_t* audio_thread_data, wchar_t* thread_name, HANDLE* thread_handle);
void AudioPause(HWAVEOUT device);
void AudioResume(HWAVEOUT device);
void AudioGetPlaybackPosition(HWAVEOUT device, LPMMTIME playback_position);
void AudioClose(HWAVEOUT device, HANDLE thread_handle, LPWAVEHDR headers, uint8_t header_count);

#endif