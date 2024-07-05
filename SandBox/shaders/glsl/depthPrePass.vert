#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 model;
} pc;

invariant gl_Position;

layout(location = 0) in vec4 inPosition;
 
void main()
{
	vec4 pos = pc.model * vec4(inPosition.xyz, 1.0f);
	gl_Position = ubo.proj * ubo.view * pos;
}