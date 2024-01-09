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

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;
    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    float alpha = texture(colorSampler, fragTexCoord).a;

    if (ALPHA_MASK) {
        if (alpha < ALPHA_MASK_CUTOFF) {
            discard;
        }
    }

    vec3 albedo = pow(texture(colorSampler, fragTexCoord).rgb, vec3(2.2));

    float metallic = texture(metallicRoughnessSampler, fragTexCoord).b;
    float roughness = texture(metallicRoughnessSampler, fragTexCoord).g;
    float ao = texture(aoSampler, fragTexCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(fragViewVec);
    
    //reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // calculate per-light radiance
    vec3 L = normalize(fragLightVec);
    vec3 H = normalize(V + L);
    float distance = length(fragLightVec);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(255.0, 255.0, 255.0) * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
    vec3 specular = nominator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // add to outgoing radiance Lo
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

    vec3 ambient = 0.03 * albedo * ao;
    vec3 col = ambient + Lo;

    // HDR tonemapping
    col = col / (col + vec3(1.0));
    
    float exposure = 0.5;
    col = pow(col, vec3(1.0 / exposure));

    outColor = vec4(col, alpha) + texture(emissionSampler, fragTexCoord);
}