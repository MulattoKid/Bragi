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

#ifndef DFT_H
#define DFT_H

#include "wav.h"

#include <windows.h>

// To avoid having to compute too many DFT windows per frame (if the frame time is high) we have a maximum
#define DFT_MAX_WINDOWS 2
// Number of samples in the window processed through each iteration of the DFT
#define DFT_N 512
// Number of frequency bands we get when using DFT_N samples
// Half of DFT_N because the other half are redundant complex conjugats
#define DFT_BAND_COUNT 256 // DFT_N / 2
// Number of "usable" frequency bands
// Band 0 is the DC-term (the 0Hz term, which is the average of all the other frequency bands in the sample window)
#define DFT_FREQUENCY_BAND_COUNT 255 // DFT_BAND_COUNT - 1

void DFTComputeWAV(wav_t* wav, DWORD sample_start, DWORD sample_end, float* frequency_bands);
void DFTComputeRAW(byte* audio_data, int32_t sample_count, int16_t bits_per_sample, int16_t bytes_per_sample_all_channels, float* frequency_bands);

#endif