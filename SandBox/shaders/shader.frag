#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D aoSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragViewVec;
layout(location = 3) in vec3 fragLightVec;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec4 fragTangent;
layout(location = 6) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0f;

const float PI = 3.1415926535897932384626433832795f;

float Reinhard2(float x) {
    const float L_white = 4.0;
    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

// ACES is troubling, I think gamma correction is built in, so resulting image is too bright

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;

    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlickRoughness(float HdotV, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - HdotV, 5.0f);
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
    //vec3 N = normalize(fragNormal);
    //vec3 T = normalize(fragTangent.xyz);
    //vec3 B = normalize(cross(N, T));
    //mat3 TBN = mat3(T, B, N);

    //return normalize(TBN * tangentNormal);
    vec3 Q1 = dFdx(fragPosition);
    vec3 Q2 = dFdy(fragPosition);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N = normalize(fragNormal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    vec4 colorval = texture(colorSampler, fragTexCoord);
    vec3 albedo = colorval.rgb;
    float alpha = colorval.a;

    if (ALPHA_MASK) {
        if (alpha < ALPHA_MASK_CUTOFF) {
            discard;
        }
    }

    float metallic = texture(metallicRoughnessSampler, fragTexCoord).b;
    float roughness = texture(metallicRoughnessSampler, fragTexCoord).g;
    float ao = texture(aoSampler, fragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(fragViewVec);
    
    //reflectance at normal incidence (base reflectivity)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    vec3 ambient = vec3(0.03f) * ao * albedo;
    for(int i = 0; i < 1; i++) {

        // calculate per-light radiance
        vec3 L = normalize(fragLightVec);
        vec3 H = normalize(V + L);
        float distance = length(fragLightVec);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = vec3(255.0f, 255.0f, 255.0f) * attenuation;

        // Cook-Torrance BRDF
        float NdotV = max(dot(N, V), 0.0000001f);
        float NdotL = max(dot(N, L), 0.0000001f);
        float HdotV = max(dot(H, V), 0.0f);
        float NdotH = max(dot(N, H), 0.0f);

        float D = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        vec3 F = fresnelSchlickRoughness(HdotV, F0, roughness);

        vec3 specular = D * G * F;
        specular /= 4.0 * NdotV * NdotL;

	vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0f - metallic;

        NdotL = max(dot(N, L), 0.0f);
        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	
    vec3 ks = F;
    vec3 kd = vec3(1.0f) - ks;
    kd *= 1.0f - metallic;

    vec3 diffuse = 0.03f * albedo; // 0.03 is camera ambient brightness

    ambient = (kd * diffuse) * ao;

    vec3 col = ambient + Lo + texture(emissionSampler, fragTexCoord).xyz;

    outColor = vec4(pow(Uncharted2Tonemap(col), vec3(2.2)), alpha);
}