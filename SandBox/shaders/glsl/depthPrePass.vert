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

layout(std430, set = 2, binding = 0) readonly buffer ModelMatrices {
	mat4 modelMatrices[];
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;

layout(location = 0) out vec2 fragTexCoord;
 
void main()
{
	fragTexCoord = vec2(inPosition.w, inNormal.w);
	gl_Position = ubo.proj * ubo.view * modelMatrices[gl_BaseInstance] * vec4(inPosition.xyz, 1.0f);
}