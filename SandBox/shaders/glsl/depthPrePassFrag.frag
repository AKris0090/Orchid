#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;

layout(location = 0) in vec2 fragTexCoord;

void main()
{
        if ((texture(colorSampler, fragTexCoord).a) < (0.5f)) {
            	discard;
        }
}