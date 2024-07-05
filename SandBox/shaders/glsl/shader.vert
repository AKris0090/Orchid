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

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inTangent;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBNMatrix;

invariant gl_Position;

void main() {
    fragTexCoord = vec2(inPosition.w, inNormal.w);

    vec4 pos = pc.model * vec4(inPosition.xyz, 1.0f);

    vec3 fragNormal = mat3(pc.model) * inNormal.xyz;
    vec4 fragTangent = vec4((pc.model * inTangent).xyz, inTangent.w);

    TBNMatrix = mat3(normalize(fragTangent.xyz), normalize(cross(inNormal.xyz, inTangent.xyz) * inTangent.w), normalize(fragNormal));

    fragPosition = vec4(pos.xyz, (ubo.view * pos).z);

    gl_Position = ubo.proj * ubo.view * pos;
}