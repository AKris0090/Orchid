#version 460

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

layout(push_constant) uniform pushConstant {
    int pCascadeIndex;
};

layout(std430, set = 1, binding = 0) readonly buffer ModelMatrices {
	mat4 modelMatrices[];
};

layout(location = 0) in vec4 inPosition;
 
void main()
{
	gl_Position =  ubo.cascadeViewProj[pCascadeIndex] * modelMatrices[gl_BaseInstance] * vec4(inPosition.xyz, 1.0);
}