#version 460

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

struct pcInstance 
{
	mat4 model;
};

layout(set = 0, binding = 1) uniform OtherUniformBuffer {
    int cascadeIndex;
} cascadeUBO;

layout(std430, set = 0, binding = 2) readonly buffer pushConstantBuffer {
	pcInstance pushConstantsArray[];
};

layout(location = 0) in vec3 inPosition;
 
void main()
{
	pcInstance drawInstance = pushConstantsArray[gl_InstanceIndex];
	gl_Position =  ubo.cascadeViewProj[cascadeUBO.cascadeIndex] * drawInstance.model * vec4(inPosition, 1.0);
}