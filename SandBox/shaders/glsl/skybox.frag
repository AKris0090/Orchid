#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 brightColor;

void main() 
{
	outFragColor = vec4(texture(samplerCubeMap, inUVW).rgb, 1.0);
	brightColor = vec4(vec3(0.0f), 1.0f);
}