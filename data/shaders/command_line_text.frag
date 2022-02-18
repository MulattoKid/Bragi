#version 450

layout (location = 0) in vec2 in_uv;

layout (binding = 0) uniform sampler2D font_sampler;

layout(std430, push_constant) uniform PushConstantLayout {
    vec4 color;
} PushConstants;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(PushConstants.color.rgb, texture(font_sampler, in_uv).r);
}