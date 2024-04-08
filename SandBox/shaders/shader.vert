#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    mat4 depthVP;
    float bias;
} ubo;

layout(push_constant) uniform pushConstant {
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragViewPos;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragLightVec;
layout(location = 4) out vec3 fragNormal;
layout(location = 5) out vec4 fragTangent;
layout(location = 6) out vec4 fragShadowCoord;
layout(location = 7) out float fragBias;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    fragTexCoord = inTexCoord;

    fragBias = ubo.bias;

    vec4 pos = pc.model * vec4(inPosition, 1.0f);
    fragPosition = pos.xyz / pos.w;

    fragViewPos = ubo.viewPos.xyz;
    fragLightVec = ubo.lightPos.xyz - fragPosition;

    fragNormal = mat3(pc.model) * inNormal;
    fragTangent = vec4((pc.model * inTangent).xyz, inTangent.w);

    fragShadowCoord = (biasMat * ubo.depthVP * pc.model) * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * vec4(fragPosition, 1.0);
}