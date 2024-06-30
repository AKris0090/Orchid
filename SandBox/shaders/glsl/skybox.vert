#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    mat4 depthMVP;
    float bias;
    int maxReflection;
} ubo;

layout(location = 0) in vec4 inPosition;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPosition.xyz;
	mat4 viewMat = mat4(mat3(ubo.view));
	gl_Position = ubo.proj * viewMat * vec4(inPosition.xyz, 1.0);
}