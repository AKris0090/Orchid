#version 450

#define SHADOW_MAP_CASCADE_COUNT 3

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
} ubo;

layout (set = 1, binding = 0) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
    float exposure = 5.0f;
    const float gamma = 2.2;
    vec3 hdrColor = texture(samplerCubeMap, inUVW).rgb;

    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * ubo.gammaExposure.y);
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / ubo.gammaExposure.x));

    outFragColor = vec4(mapped, 1.0);
}