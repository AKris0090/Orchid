#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 model;
    uint cascadeIndex;
} pc;

layout(location = 0) in vec3 inPosition;
 
void main()
{
	gl_Position =  ubo.cascadeViewProj[pc.cascadeIndex] * pc.model * vec4(inPosition, 1.0);
}