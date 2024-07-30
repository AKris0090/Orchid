#version 460

#include "aces.glsl"

#define SHADOW_MAP_CASCADE_COUNT 3

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
    float specularCont;
} ubo;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D colorSampler;

void main()
{	
	outColor = vec4(1.0) - exp(-texture(colorSampler, texCoords) * ubo.gammaExposure.y);
	outColor = pow(outColor, vec4(1.0 / ubo.gammaExposure.x));
	outColor = vec4(ACESFilm(outColor.rgb), 1.0f);
}