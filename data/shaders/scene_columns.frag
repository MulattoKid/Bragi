#version 450

#define DFT_FREQUENCY_BAND_COUNT 255 // DFT_BAND_COUNT - 1 -> skip DC-term
#define DFT_FREQUENCY_BAND_MAX_ARRAY_INDEX 254

layout (location = 0) in vec2 in_uv;

layout (set = 0, binding = 0, std430) buffer DFTBufferLayout {
    float frequency_band_magnitudes[DFT_FREQUENCY_BAND_COUNT]; // Matches DFT_FREQUENCY_BAND_COUNT
} DFTBuffer;

layout(std430, push_constant) uniform PushConstantLayout {
    vec2 resolution;
} PushConstants;

layout (location = 0) out vec4 out_color;

void main()
{
    vec2 fragment_position = vec2(gl_FragCoord.x / PushConstants.resolution.x, ((gl_FragCoord.y / PushConstants.resolution.y) * (-1.0f)) + 1.0f);

    // Get frequency band for column
    int band = min(int(fragment_position.x * 255.0f), DFT_FREQUENCY_BAND_MAX_ARRAY_INDEX); // [0,DFT_FREQUENCY_BAND_MAX_ARRAY_INDEX]
    float frequency_band_magnitude = DFTBuffer.frequency_band_magnitudes[band];
    vec3 color = vec3(1.0f, 0.0f, 0.0f);
    if (fragment_position.y <= frequency_band_magnitude)
    {
        color *= frequency_band_magnitude;
    }
    else
    {
        color = vec3(0.0f);
    }

    out_color = vec4(color, 1.0f);
}