#version 460

#define SHADOW_MAP_CASCADE_COUNT 3

#include "aces.glsl"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 gammaExposure;
    vec4 cascadeSplits;
    mat4 cascadeViewProj[SHADOW_MAP_CASCADE_COUNT];
} ubo;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

layout(push_constant) uniform pushConstant {
    layout(offset = 64) bool isOutline;
} pc;

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D aoSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;
layout(set = 1, binding = 5) uniform sampler2D brdfTexture;
layout(set = 1, binding = 6) uniform samplerCube irradianceCube;
layout(set = 1, binding = 7) uniform samplerCube prefilteredEnvMap;
layout(set = 1, binding = 8) uniform sampler2DArray samplerDepthMap;

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 TBNMatrix;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 bloomColor;
layout (depth_unchanged) out float gl_FragDepth;

vec4 albedoAlpha = texture(colorSampler, fragTexCoord);
vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
vec3 aoVec = texture(aoSampler, fragTexCoord).rrr;
vec3 emissionVec = texture(emissionSampler, fragTexCoord).rgb;

#define PI 3.1415926535897932384626433832795
#define ALBEDO albedoAlpha.rgb
#define ALPHA albedoAlpha.a

vec3 calculateNormal()
{
	return normalize(TBNMatrix * tangentNormal);
}

float ProjectUV(vec4 shadowCoord, vec2 off, uint cascadeIndex, float newBias)
{
	float dist = texture( samplerDepthMap, vec3(shadowCoord.st + off, cascadeIndex)).r;

	if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - newBias ) 
	{
		return 0.0;
	}

	return 1.0;
}

const int range = 1;
const int kernelRange = (2 * range + 1) * (2 * range + 1);

float ShadowCalculation(vec4 fragPosLightSpace, uint cascadeIndex, float newBias)
{
	ivec2 texDim = textureSize(samplerDepthMap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += ProjectUV(fragPosLightSpace, vec2(dx*x, dy*y), cascadeIndex, newBias);
		}
	
	}

	return shadowFactor / (kernelRange);
}

const vec3 lightColor = vec3(0.7333f, 0.8666f, 0.9921f);

//vec3 mapped = ACESFilm(color * ubo.gammaExposure.y);//mapped = mapped * (1.0f / ACESFilm(vec3(11.2)));//mapped = pow(mapped, vec3(1.0 / ubo.gammaExposure.x));

void main()
{
	vec3 N = calculateNormal();

	vec3 V = normalize(ubo.viewPos.xyz - fragPosition.xyz);
	vec3 L = normalize(ubo.lightPos.xyz - fragPosition.xyz);

	vec3 res = step(ubo.cascadeSplits.xyz, vec3(fragPosition.w));
	int cascadeIndex = SHADOW_MAP_CASCADE_COUNT - int(res.x + res.y + res.z);

	float newBias = max(0.05 * (1.0 - dot(N, ubo.lightPos.xyz)), ubo.viewPos.w);

	newBias *= 1 / (ubo.cascadeSplits[cascadeIndex] * 0.5);

	vec4 fragShadowCoord = (biasMat * ubo.cascadeViewProj[cascadeIndex]) * vec4(fragPosition.xyz, 1.0);

	float shadow = ShadowCalculation((fragShadowCoord / fragShadowCoord.w), cascadeIndex, newBias);
	
	float ambientVAL = 1.0f;

	float inner = pow(clamp((((1.0 - max(dot(N, V), 0.0f))) / 0.65f), 0.0f, 1.0f), 30.0f) * ubo.gammaExposure.z;

	vec3 color = vec3(ambientVAL + (inner * clamp(dot(N, L), 0.0f, 1.0f)));

	color *= ALBEDO * max(0.5f, (shadow)) * max(1.0f, (shadow * 0.2f));

	vec4 brightColor = vec4(color, 1.0f);

	if(dot(brightColor.rgb, vec3(0.2126, 0.7152, 0.0722)) > 1.0f) {
		bloomColor = vec4(brightColor.rgb, 1.0f);
	} else {
		bloomColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	outColor = vec4(color, ALPHA);
}