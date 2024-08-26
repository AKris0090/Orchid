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
    float specularCont;
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 model;
} pc;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec4 secondTexCoord;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec4 fragTexCoord;
layout(location = 2) out mat3 TBNMatrix;

invariant gl_Position;

void main() {
    fragTexCoord = vec4(inPosition.w, inNormal.w, secondTexCoord.x, secondTexCoord.y);

    vec4 pos = pc.model * vec4(inPosition.xyz, 1.0f);

    TBNMatrix = mat3(normalize((vec4((pc.model * inTangent).xyz, inTangent.w)).xyz), normalize(cross(inNormal.xyz, inTangent.xyz) * inTangent.w), normalize((mat3(pc.model) * inNormal.xyz)));

    fragPosition = vec4(pos.xyz, (ubo.view * pos).z);

    gl_Position = ubo.proj * ubo.view * pos;
}