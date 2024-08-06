#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
} ubo;

layout(location = 0) in vec4 inPosition;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPosition.xyz;
	gl_Position = ubo.proj * mat4(mat3(ubo.view)) * vec4(inPosition.xyz, 1.0);
}