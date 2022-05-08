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

#include "macros.h"
#include "dft.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define MATH_PI 3.14159265359f
#define MATH_TWO_PI 6.28318530718f

// TODO (Daniel): optimize
void DFTCompute(wav_t* wav, DWORD sample_start, DWORD sample_end, float* frequency_bands)
{
    static float dft_real[DFT_N];
    static float dft_imaginary[DFT_N];
    memset(dft_real, 0, DFT_N * sizeof(float));
    memset(dft_imaginary, 0, DFT_N * sizeof(float));

    // Offsets into actual audio data
    byte* audio_data_start = wav->audio_data + (sample_start * wav->bps * wav->channel_count);
    byte* audio_data_end = wav->audio_data + (sample_end * wav->bps * wav->channel_count);

    const int32_t sample_count = (int32_t)sample_end - (int32_t)sample_start;
    const int32_t iteration_count = (sample_count + DFT_N - 1) / DFT_N; // Round up
    float sample_max_value;
    if (wav->bps == 1)
    {
        sample_max_value = (float)UINT8_MAX;
    }
    else // wav->bps == 2
    {
        sample_max_value = (float)INT16_MAX;
    }
    // For each iteration
    for (int32_t i = 0; i < iteration_count; i++)
    {
        // For each bin
        for (int32_t k = 0; k < DFT_N; k++)
        {
            float real = 0.0f;
            float imaginary = 0.0f;
            // For each sample
            for (int32_t n = 0; n < DFT_N; n++)
            {
                int32_t sample_index = (i * DFT_N) + n;
                if (sample_index <= sample_count)
                {
                    int16_t* sample_left = (int16_t*)(audio_data_start + (n * wav->bps * wav->channel_count));
                    int16_t* sample_right = sample_left + 1;
                    float sample_left_f = (float)*sample_left / sample_max_value;
                    float sample_right_f = (float)*sample_right / sample_max_value;
                    float sample_avg = (sample_left_f + sample_right_f) * 0.5f; // / 2.0f

                    real += sample_avg * cosf((MATH_TWO_PI * (float)k * (float)n) / (float)DFT_N);
                    imaginary -= sample_avg * sinf((MATH_TWO_PI * (float)k * (float)n) / (float)DFT_N);
                }
                else
                {
                    // No more valid samples in window, so all remaining results will be 0.0
                    break;
                }
            }

            // Average in-place
            dft_real[k] += fabs(real) / float(iteration_count);
            dft_imaginary[k] += fabs(imaginary) / float(iteration_count);
        }
    }

    // Compute frequency magnitude for bins
    // i = 1 -> skip DC-term
    for (int32_t i = 1; i < DFT_BAND_COUNT; i++)
    {
        float real = dft_real[i];
        float imaginary = dft_imaginary[i];
        float magnitude = 2.0f * sqrtf((real * real) + (imaginary * imaginary)) / float(DFT_N);

        // In order to get more smooth drops in the magnitude, we don't jump straigt from the current
        // to the next one if it's lower than the current magnitude. Instead we scale down the current
        // magnitude, and check that we haven't gone too far
        float magnitude_scaling = 0.75f;
        float current_magnitude = frequency_bands[i - 1];
        if (magnitude >= current_magnitude)
        {
            current_magnitude = magnitude;
        }
        else
        {
            // Scale
            current_magnitude *= magnitude_scaling;

            // Ensure we haven't gone past the new magnitude
            if (current_magnitude < magnitude)
            {
                current_magnitude = magnitude;
            }
        }
        frequency_bands[i - 1] = current_magnitude;
    }
}

void DFTCompute(byte* audio_data, int32_t sample_count, int16_t bps, int16_t bytes_per_sample_all_channels, float* frequency_bands)
{
    static float dft_real[DFT_N];
    static float dft_imaginary[DFT_N];
    memset(dft_real, 0, DFT_N * sizeof(float));
    memset(dft_imaginary, 0, DFT_N * sizeof(float));

    // Offsets into actual audio data
    const int32_t iteration_count = (sample_count + DFT_N - 1) / DFT_N; // Round up
    float sample_max_value;
    if (bps == 1)
    {
        sample_max_value = (float)UINT8_MAX;
    }
    else // bps == 2
    {
        sample_max_value = (float)INT16_MAX;
    }
    // For each iteration
    for (int32_t i = 0; i < iteration_count; i++)
    {
        // For each bin
        for (int32_t k = 0; k < DFT_N; k++)
        {
            float real = 0.0f;
            float imaginary = 0.0f;
            // For each sample
            for (int32_t n = 0; n < DFT_N; n++)
            {
                int32_t sample_index = (i * DFT_N) + n;
                if (sample_index <= sample_count)
                {
                    int16_t* sample_left = (int16_t*)(audio_data + (n * bytes_per_sample_all_channels));
                    int16_t* sample_right = sample_left + 1;
                    float sample_left_f = (float)*sample_left / sample_max_value;
                    float sample_right_f = (float)*sample_right / sample_max_value;
                    float sample_avg = (sample_left_f + sample_right_f) * 0.5f; // / 2.0f

                    real += sample_avg * cosf((MATH_TWO_PI * (float)k * (float)n) / (float)DFT_N);
                    imaginary -= sample_avg * sinf((MATH_TWO_PI * (float)k * (float)n) / (float)DFT_N);
                }
                else
                {
                    // No more valid samples in window, so all remaining results will be 0.0
                    break;
                }
            }

            // Average in-place
            dft_real[k] += fabs(real) / float(iteration_count);
            dft_imaginary[k] += fabs(imaginary) / float(iteration_count);
        }
    }

    // Compute frequency magnitude for bins
    // i = 1 -> skip DC-term
    for (int32_t i = 1; i < DFT_BAND_COUNT; i++)
    {
        float real = dft_real[i];
        float imaginary = dft_imaginary[i];
        float magnitude = 2.0f * sqrtf((real * real) + (imaginary * imaginary)) / float(DFT_N);

        // In order to get more smooth drops in the magnitude, we don't jump straigt from the current
        // to the next one if it's lower than the current magnitude. Instead we scale down the current
        // magnitude, and check that we haven't gone too far
        float magnitude_scaling = 0.75f;
        float current_magnitude = frequency_bands[i - 1];
        if (magnitude >= current_magnitude)
        {
            current_magnitude = magnitude;
        }
        else
        {
            // Scale
            current_magnitude *= magnitude_scaling;

            // Ensure we haven't gone past the new magnitude
            if (current_magnitude < magnitude)
            {
                current_magnitude = magnitude;
            }
        }
        current_magnitude = magnitude;
        frequency_bands[i - 1] = current_magnitude;
    }
}