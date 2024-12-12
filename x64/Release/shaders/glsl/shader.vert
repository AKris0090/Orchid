#version 460

#define SHADOW_MAP_CASCADE_COUNT 4

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
    vec4 cascadeBiases;
} ubo;

layout(std430, set = 2, binding = 0) readonly buffer ModelMatrices {
	mat4 modelMatrices[];
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inTangent;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBNMatrix;

void main() {
    fragTexCoord = vec2(inPosition.w, inNormal.w);

    vec4 pos = modelMatrices[gl_BaseInstance] * vec4(inPosition.xyz, 1.0f);

    TBNMatrix = mat3(normalize((vec4((modelMatrices[gl_BaseInstance] * inTangent).xyz, inTangent.w)).xyz), normalize(cross(inNormal.xyz, inTangent.xyz) * inTangent.w), normalize((mat3(modelMatrices[gl_BaseInstance]) * inNormal.xyz)));

    fragPosition = vec4(pos.xyz, (ubo.view * pos).z);

    gl_Position = ubo.proj * ubo.view * pos;
}