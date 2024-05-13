#version 450

layout(push_constant) uniform pushConstant {
    mat4 vp;
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
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

	gl_Position =  pc.vp * pc.model * skinMat * vec4(inPosition, 1.0);
}