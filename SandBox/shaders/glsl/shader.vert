#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec3 inBiTangent;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBNMatrix;

void main() {
    fragTexCoord = inTexCoord;

    vec4 pos = pc.model * vec4(inPosition, 1.0f);
    vec3 fragPositionTemp = pos.xyz / pos.w;

    vec3 fragNormal = mat3(pc.model) * inNormal;
    vec4 fragTangent = vec4((pc.model * inTangent).xyz, inTangent.w);

    TBNMatrix = mat3(normalize(fragTangent.xyz), inBiTangent, normalize(fragNormal));

    fragPosition = vec4(fragPositionTemp, (ubo.view * vec4(fragPositionTemp, 1.0f)).z);

    gl_Position = ubo.proj * ubo.view * vec4(fragPositionTemp, 1.0);
}