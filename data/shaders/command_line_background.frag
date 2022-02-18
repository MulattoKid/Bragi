#version 450

layout(std430, push_constant) uniform PushConstantLayout {
    vec4 color;
} PushConstants;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(PushConstants.color);
}