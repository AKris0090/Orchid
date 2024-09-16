#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = vec4(texture(samplerCubeMap, inUVW).rgb, 1.0);
}