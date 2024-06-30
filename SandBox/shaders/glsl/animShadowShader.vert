#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 pcModel;
    uint pCascadeIndex;
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 jointIndex;
layout(location = 2) in vec4 jointWeight;

layout(std430, set = 0, binding = 0) readonly buffer JointMatrices {
	mat4 jointMatrices[];
};
 
void main()
{
	mat4 skinMat =  
		jointWeight.x * jointMatrices[int(jointIndex.x)] +
		jointWeight.y * jointMatrices[int(jointIndex.y)] +
		jointWeight.z * jointMatrices[int(jointIndex.z)] +
		jointWeight.w * jointMatrices[int(jointIndex.w)];

	gl_Position =  ubo.cascadeViewProj[pCascadeIndex] * pcModel * skinMat * vec4(inPosition.xyz, 1.0);
}