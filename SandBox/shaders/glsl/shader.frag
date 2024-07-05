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

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

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
layout (depth_unchanged) out float gl_FragDepth;

vec4 albedoAlpha = texture(colorSampler, fragTexCoord);
vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
vec3 aoVec = texture(aoSampler, fragTexCoord).rrr;
vec3 emissionVec = texture(emissionSampler, fragTexCoord).rgb;

#define PI 3.1415926535897932384626433832795
#define ALBEDO albedoAlpha.rgb
#define ALPHA albedoAlpha.a
#define AMBIENT 0.1

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a2     = roughness*roughness*roughness*roughness;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

// Fresnel function ----------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	float lod = roughness * ubo.lightPos.w;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilteredEnvMap, R, lodf).rgb;
	vec3 b = textureLod(prefilteredEnvMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = DistributionGGX(N, H, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = GeometrySmith(N, V, L, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = fresnelSchlick(dotNV, F0);		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		return (kD * ALBEDO / PI + spec) * dotNL;
	} else {
		return vec3(0.0);
	}
}

vec3 calculateNormal()
{
	return normalize(TBNMatrix * tangentNormal);
}

float ProjectUV(vec4 shadowCoord, vec2 off, uint cascadeIndex, float newBias)
{
	float dist = texture( samplerDepthMap, vec3(shadowCoord.st + off, cascadeIndex)).r;

	if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - newBias ) 
	{
		return AMBIENT;
	}

	return 1.0;
}

ivec2 texDim = textureSize(samplerDepthMap, 0).xy;
const int range = 1;
const int kernelRange = (2 * range + 1) * (2 * range + 1);

float ShadowCalculation(vec4 fragPosLightSpace, uint cascadeIndex, float newBias)
{
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

void main()
{
	vec3 N = calculateNormal();

	vec3 V = normalize(ubo.viewPos.xyz - fragPosition.xyz);
	vec3 L = normalize(ubo.lightPos.xyz - fragPosition.xyz);
	vec3 R = -normalize(reflect(V, N));

	float NdotV = max(dot(N, V), 0.0);
	//float NdotL = max(dot(N, L), 0.0);
	//float NdotH = max(dot(N, V + L), 0.0);

	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);

	vec3 Lo = specularContribution(L, V, N, F0, metallic, roughness);

	vec2 brdf = texture(brdfTexture, vec2(NdotV, roughness)).rg;
        vec3 reflected =  prefilteredReflection(R, roughness).rgb;
        vec3 irradiance = texture(irradianceCube, N).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * ALBEDO;

        vec3 F = F_SchlickR(NdotV, F0, roughness);	

	// Specular reflectance
	vec3 specular = reflected * (F * brdf.x + brdf.y);

	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	
	vec3 ambient = (kD * diffuse + specular) * aoVec;

	// Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(fragPosition.w < ubo.cascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}

	float newBias = max(0.05 * (1.0 - dot(N, ubo.lightPos.xyz)), ubo.viewPos.w);

	if(cascadeIndex == SHADOW_MAP_CASCADE_COUNT) {
		newBias *= 1 / (((ubo.proj[2][3] / ubo.proj[2][2]) + 1.0) * 0.5);
	} else {
		newBias *= 1 / (ubo.cascadeSplits[cascadeIndex] * 0.5);
	}

	vec4 fragShadowCoord = (biasMat * ubo.cascadeViewProj[cascadeIndex]) * vec4(fragPosition.xyz, 1.0);

	float shadow = ShadowCalculation((fragShadowCoord / fragShadowCoord.w), cascadeIndex, newBias);
	
	vec3 color = ((ambient + (Lo)) * shadow) + emissionVec;

	// Tone mapping
	//color = Uncharted2Tonemap(color * 2.5f); // 4.5f is exposure
	//color = color * (1.0f / Uncharted2Tonemap(vec3(9.0f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / 2.2f)); // 2.2 is gamma

	outColor = vec4(color, ALPHA);
}