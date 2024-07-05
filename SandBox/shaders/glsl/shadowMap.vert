#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 pCModel;
    uint pCascadeIndex;
};

layout(location = 0) in vec4 inPosition;
 
void main()
{
	gl_Position =  ubo.cascadeViewProj[pCascadeIndex] * pCModel * vec4(inPosition.xyz, 1.0);
}