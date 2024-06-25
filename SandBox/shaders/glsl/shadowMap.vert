#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

struct pcInstance 
{
	mat4 model;
	uint cascadeIndex;
};

layout(std430, set = 0, binding = 1) readonly buffer pushConstantBuffer {
	pcInstance pushConstants[];
};

layout(location = 0) in vec3 inPosition;
 
void main()
{
	pcInstance drawInstance = pushConstants[gl_InstanceIndex];
	gl_Position =  ubo.cascadeViewProj[drawInstance.cascadeIndex] * drawInstance.model * vec4(inPosition, 1.0);
}