#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;

layout(push_constant) uniform pushConstant {
    layout(offset = 64) float alphaCutoff;
} pc;

layout(location = 0) in vec2 fragTexCoord;

void main()
{
        if ((texture(colorSampler, fragTexCoord).a) < (pc.alphaCutoff)) {
            	discard;
        }
}