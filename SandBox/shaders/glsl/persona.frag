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

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 8) uniform sampler2DArray samplerDepthMap;

layout(std430, set = 2, binding = 0) readonly buffer ModelMatrices {
	mat4 modelMatrices[];
};

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec4 fragNormal;
layout(location = 2) in vec4 fragTangent;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 bloomColor;

vec4 albedoAlpha = texture(colorSampler, fragTexCoord);
vec3 tangentNormal = normalize((texture(normalSampler, fragTexCoord).xyz * 2.0) - 1.0);

#define PI 3.1415926535897932384626433832795
#define ALBEDO albedoAlpha.rgb
#define ALPHA albedoAlpha.a

vec3 calculateNormal()
{
	return mat3(fragTangent.xyz, (normalize(cross(fragNormal.xyz, fragTangent.xyz)) * fragTangent.w), fragNormal.xyz) * tangentNormal;
}

float ProjectUV(vec4 shadowCoord, vec2 off, uint cascadeIndex, float newBias)
{
	float dist = texture( samplerDepthMap, vec3(shadowCoord.st + off, cascadeIndex)).r;

	if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - newBias ) 
	{
		return 0.0f;
	}

	return 1.0f;
}

const int range = 2;
const int kernelRange = (2 * range + 1) * (2 * range + 1);

float ShadowCalculation(vec4 fragPosLightSpace, uint cascadeIndex, float newBias)
{
	ivec2 texDim = textureSize(samplerDepthMap, 0).xy;
	float scale = 0.5f;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += ProjectUV(fragPosLightSpace, vec2(dx*(x), dy*(y)), cascadeIndex, newBias);
		}
	
	}

	return shadowFactor / (kernelRange);
}

vec3 lightColor = (vec3(244.0f, 215.0f, 159.0f) / 255.0f);

void main()
{
	vec3 N = calculateNormal();
	vec3 V = normalize(ubo.viewPos.xyz - fragPosition.xyz);
	vec3 L = normalize(ubo.lightPos.xyz - fragPosition.xyz);

	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(fragPosition.w < ubo.cascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}

	ivec2 texDim = textureSize(samplerDepthMap, 0).xy;
	float texelSize = 1.0f / float(texDim.x);
	float LdotN = dot(L, fragNormal.xyz);
	float normalOffsetScale = clamp((1.0 - LdotN), 0.0f, 1.0f);
	normalOffsetScale *= ubo.cascadeBiases[cascadeIndex] * texelSize;
	vec3 shadowOffset = fragNormal.xyz * normalOffsetScale;

	vec4 fragShadowCoord = (biasMat * ubo.cascadeViewProj[cascadeIndex]) * vec4(fragPosition.xyz + shadowOffset, 1.0f);

	float shadow = ShadowCalculation((fragShadowCoord / fragShadowCoord.w), cascadeIndex, 0.0f);

	float inner = pow(clamp((((1.0 - max(dot(N, V), 0.0f))) / 0.65f), 0.0f, 1.0f), 30.0f);

	float altShadow = clamp(pow(shadow + 0.8f, 0.6f), 0.0f, 1.0f) * 2.3f;

	vec3 color = (ALBEDO * altShadow) + (clamp(dot(N, L), 0.0f, 1.0f) * (inner * lightColor));

	color = mix(vec3(135.0f / 255.0f, 135.0f / 255.0f, 255.0f / 255.0f) * color, color, shadow);
	color = mix(vec3(255.0f / 255.0f, 215.0f / 255.0f, 195.0f / 255.0f) * color, color, 1.0f - shadow);

	color = pow(color, vec3(1.0 / ubo.gammaExposure.x));

	bloomColor = vec4(vec3((clamp(dot(N, L), 0.0f, 1.0f) * (inner * lightColor))), 1.0f) * shadow * 0.5f;

	outColor = vec4(color, ALPHA);
}