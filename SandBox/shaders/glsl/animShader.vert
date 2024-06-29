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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec3 inBiTangent;
layout(location = 5) in vec4 jointIndex;
layout(location = 6) in vec4 jointWeight;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 TBNMatrix;

layout(std430, set = 2, binding = 0) readonly buffer JointMatrices {
	mat4 jointMatrices[];
};

void main() {
    mat4 skinMat =  
		jointWeight.x * jointMatrices[int(jointIndex.x)] +
		jointWeight.y * jointMatrices[int(jointIndex.y)] +
		jointWeight.z * jointMatrices[int(jointIndex.z)] +
		jointWeight.w * jointMatrices[int(jointIndex.w)];

    fragTexCoord = inTexCoord;

    vec4 pos = pc.model * skinMat * vec4(inPosition, 1.0f);
    vec3 fragPositionTemp = pos.xyz / pos.w;

    vec3 fragNormal = mat3(pc.model * skinMat) * inNormal;
    vec4 fragTangent = vec4((pc.model * skinMat * inTangent).xyz, inTangent.w);

    TBNMatrix = mat3(normalize(fragTangent.xyz), inBiTangent, normalize(fragNormal));

    fragPosition = vec4(fragPositionTemp, (ubo.view * vec4(fragPositionTemp, 1.0f)).z);

    gl_Position = ubo.proj * ubo.view * vec4(fragPositionTemp, 1.0);
}