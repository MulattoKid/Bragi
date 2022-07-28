#include "audio.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265359f
#define HAMMING 0.53836f

// https://en.wikipedia.org/wiki/Euclidean_algorithm#Implementations
uint32_t FindGreatestCommonDivisor(uint32_t a, uint32_t b)
{
    int32_t tmp;

    while (b != 0)
    {
        tmp = b;
        b = a % b;
        a = tmp;
    }

    return a;
}

void LowPassFilterCreate(const uint32_t input_rate, const uint32_t upsampling_factor, const uint32_t filter_length, float* filter, const filter_type_t filter_type, window_type_t const window_type)
{
    #define max_filter_length 256
    assert(filter_length <= max_filter_length);

    // Filter properties
    const uint32_t filter_shift = filter_length / 2;
    const uint32_t filter_sample_rate = input_rate * upsampling_factor;
    const float filter_sample_delta = 1.0f / (float)filter_sample_rate;
    const float cutoff_freq = input_rate / 2;

    // Create window
    float window[max_filter_length];
    for (int32_t i = 0; i < filter_length; i++)
    {
        switch (window_type)
        {
            case WINDOW_TYPE_RECTANGULAR:
            {
                // https://en.wikipedia.org/wiki/Window_function#Rectangular_window
                window[i] = 1.0f;
            } break;

            case WINDOW_TYPE_HAMMING:
            {
                // https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
                window[i] = HAMMING - ((1.0 - HAMMING) * cosf((2.0 * PI * (float)i) / (float)(filter_length - 1)));
            } break;

            default:
            {
                assert(0);
            } break;
        }
    }

    // Create filter
    float filter_sum = 0.0f;
    for (int32_t i = 0; i < filter_length; i++)
    {
        int32_t sample_index = i - filter_shift;
        float sample_time = (float)sample_index * filter_sample_delta;

        switch (filter_type)
        {
            case FILTER_TYPE_SINC:
            {
                float sinc_value = 2.0f * cutoff_freq;
                if (sample_index != 0)
                {
                    // https://en.wikipedia.org/wiki/Sinc_function
                    sinc_value = sinf(2.0 * PI * cutoff_freq * sample_time) / (PI * sample_time);
                }
                filter[i] = sinc_value;
            } break;

            default:
            {
                assert(0);
            } break;
        }
    }

    // Apply window
    for (int32_t i = 0; i < filter_length; i++)
    {
        filter[i] *= window[i];
        filter_sum += fabs(filter[i]);
    }

    // Normalize filter
    for (int32_t i = 0; i < filter_length; i++)
    {
        filter[i] /= filter_sum;
    }
}

uint32_t SampleRateConvert(const uint32_t input_rate, const uint32_t output_rate, const uint32_t upsampling_factor, const uint32_t decimation_factor, const float slow_down_factor, const uint32_t sample_count_all_channels, const uint8_t bps, const uint8_t channel_count, const byte_t* audio_data, const byte_t* upsampled_audio_data, const byte_t* prefetch_buffer, const uint32_t filter_length, float* filter, const byte_t* upsampled_audio_data_with_prefetch_buffer, const byte_t* upsampled_audio_data_filtered, const byte_t* upsampled_audio_data_final)
{
    const uint32_t sample_count_per_channel = sample_count_all_channels / channel_count;
    const uint32_t sample_count_upsampled_per_channel = sample_count_per_channel * upsampling_factor;
    const uint32_t sample_count_output_per_channel = sample_count_upsampled_per_channel / decimation_factor;
    const uint32_t sample_count_upsampled_all_channels = sample_count_upsampled_per_channel * channel_count;
    const uint32_t sample_count_output_all_channels = sample_count_output_per_channel * channel_count;

    // Upsample by zero-stuffing
    memset((void*)upsampled_audio_data, 0, sample_count_upsampled_all_channels * bps);
    for (uint8_t channel = 0; channel < channel_count; channel++)
    {
        int32_t channel_offset = channel * bps;
        for (int32_t i = 0; i < sample_count_per_channel; i++)
        {
            memcpy((void*)(upsampled_audio_data + (i * channel_count * upsampling_factor * bps) + channel_offset), (void*)(audio_data + (i * channel_count * bps) + channel_offset), bps);
        }
    }

    // Copy filter_length samples from the prefetch buffer into the start of the actual buffer to be used for filtering
    memcpy((void*)upsampled_audio_data_with_prefetch_buffer, (void*)prefetch_buffer, filter_length * channel_count * bps);
    // Copy the first N - filter_length samples into the actual buffer to be used for filtering at an offset
    memcpy((void*)(upsampled_audio_data_with_prefetch_buffer + (filter_length * channel_count * bps)), (void*)upsampled_audio_data, sample_count_upsampled_all_channels * bps);
    // Copy the last filter_length samples into the prefetch buffer for the next iteration
    memcpy((void*)prefetch_buffer, (void*)(upsampled_audio_data + (sample_count_upsampled_all_channels * bps) - (filter_length * channel_count * bps)), filter_length * channel_count * bps);

    // Filter
    assert(bps == 2); // int16_t samples
    for (uint8_t channel = 0; channel < channel_count; channel++)
    {
        for (int32_t i = 0; i < sample_count_upsampled_per_channel; i++)
        {
            float filtered_sample = 0.0f;
            // TODO: optimize again
            /*int32_t j_offset = L - (i % L);
            for (int32_t j = j_offset; j < filter_length_for_sample; j += L)
            {
                filtered_sample += (float)((int16_t*)upsampled_audio_data_with_prefetch_buffer)[current_sample_index + j] * filter[j];
            }*/
            // Don't optimize for now, since we don't know which index our first non-zero-stuffed value is in in our 'proper' upsampled buffer
            for (int32_t j = 0; j < filter_length; j++)
            {
                filtered_sample += (float)((int16_t*)upsampled_audio_data_with_prefetch_buffer)[(i * channel_count) + (j * channel_count) + channel] * filter[j];
            }
            ((int16_t*)upsampled_audio_data_filtered)[(i * channel_count) + channel] = filtered_sample;
        }
    }

    // Decimate
    assert(bps == 2); // int16_t samples
    for (uint8_t channel = 0; channel < channel_count; channel++)
    {
        uint32_t channel_offset_src = sample_count_upsampled_per_channel * channel;
        uint32_t channel_offset_dst = sample_count_output_per_channel * channel;
        for (int32_t i = 0; i < sample_count_output_per_channel; i++)
        {
            ((int16_t*)upsampled_audio_data_final)[(i * channel_count) + channel] = ((int16_t*)upsampled_audio_data_filtered)[(i * decimation_factor * channel_count) + channel];
        }
    }

    return sample_count_output_all_channels;
}