#version 460

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
    vec4 cascadeBiases;
} ubo;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(vec3(0.0f), 1.0f);
}