#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;

layout(push_constant) uniform pushConstant {
    layout(offset = 64) int alphaMask;
    layout(offset = 68) float alphaCutoff;
} pc;

layout(location = 0) in vec2 fragTexCoord;

void main()
{
	if (pc.alphaMask > 0) {
        	if ((texture(colorSampler, fragTexCoord).a) < (pc.alphaCutoff + 0.38)) {
            		discard;
        	}
	}
}