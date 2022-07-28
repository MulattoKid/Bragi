#ifndef AUDIO_H
#define AUDIO_H

#include "macros.h"

#include <stdint.h>

typedef enum
{
    FILTER_TYPE_SINC = 0
} filter_type_t;

typedef enum
{
    WINDOW_TYPE_RECTANGULAR = 0,
    WINDOW_TYPE_HAMMING     = 1
} window_type_t;

uint32_t FindGreatestCommonDivisor(uint32_t a, uint32_t b);
void     LowPassFilterCreate(const uint32_t input_rate, const uint32_t upsampling_factor, const uint32_t filter_length, float* filter, const filter_type_t filter_type, const window_type_t window_type);
uint32_t SampleRateConvert(const uint32_t input_rate, const uint32_t output_rate, const uint32_t upsampling_factor, const uint32_t decimation_factor, const float slow_down_factor, const uint32_t sample_count_all_channels, const uint8_t bps, const uint8_t channel_count, const byte_t* audio_data, const byte_t* upsampled_audio_data, const byte_t* prefetch_buffer, const uint32_t filter_length, float* filter, const byte_t* upsampled_audio_data_with_prefetch_buffer, const byte_t* upsampled_audio_data_filtered, const byte_t* upsampled_audio_data_final);

#endif