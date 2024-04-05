#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 3) uniform sampler2D aoSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;

layout(set = 1, binding = 5) uniform sampler2D samplerCubeMap;
layout(set = 1, binding = 6) uniform samplerCube irradianceCube;
layout(set = 1, binding = 7) uniform samplerCube prefilteredEnvMap;

layout(set = 1, binding = 8) uniform sampler2D samplerDepthMap;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragViewPos;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragLightVec;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec4 fragTangent;
layout(location = 6) in vec4 fragShadowCoord;

layout(location = 0) out vec4 outColor;

float ambient = 0.1f;

layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0f;

#define PI 3.1415926535897932384626433832795
#define ALBEDO texture(colorSampler, fragTexCoord).rgb
#define ALPHA texture(colorSampler, fragTexCoord).a

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
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
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
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
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
		color += (kD * ALBEDO / PI + spec) * dotNL;
	}

	return color;
}

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalSampler, fragTexCoord).xyz * 2.0 - 1.0;


        vec3 T = normalize(fragTangent.xyz);
        vec3 B = normalize(cross(fragNormal.xyz, fragTangent.xyz) * fragTangent.w);
        vec3 N = normalize(fragNormal);

	return normalize(mat3(T, B, N) * tangentNormal);
}

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( samplerDepthMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = ambient;
		}
	}
	return shadow;
}

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    vec3 projCoords = vec3(fragPosLightSpace / fragPosLightSpace.w);
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(samplerDepthMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  

    return shadow;
}

void main()
{		
	if (ALPHA_MASK) {
        	if (ALPHA < ALPHA_MASK_CUTOFF) {
            		discard;
        	}
   	}

	vec3 N = calculateNormal();

	vec3 V = normalize(fragViewPos - fragPosition);
	vec3 R = -normalize(reflect(V, N)); 

	float metallic = texture(metallicRoughnessSampler, fragTexCoord).b;
	float roughness = texture(metallicRoughnessSampler, fragTexCoord).g;

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 1; i++) {
		vec3 L = normalize(fragLightVec);
                Lo += specularContribution(L, V, N, F0, metallic, roughness);
	}  

	float bias = max(0.05 * (1.0 - dot(N, normalize(fragLightVec))), 0.005);  

	float shadow = ShadowCalculation(fragShadowCoord, bias);

	vec2 brdf = texture(samplerCubeMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 reflected =  prefilteredReflection(R, roughness).rgb;
        vec3 irradiance = texture(irradianceCube, N).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * ALBEDO;
	diffuse *= (1.0 - shadow);

        vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);	

	// Specular reflectance
	vec3 specular = reflected * (F * brdf.x + brdf.y);

	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	
	vec3 ambient = (kD * diffuse + specular) * texture(aoSampler, fragTexCoord).rrr;
	
	vec3 color = (ambient + (Lo)) + texture(emissionSampler, fragTexCoord).rgb;

	// Tone mapping
	//color = Uncharted2Tonemap(color * 2.5f); // 4.5f is exposure
	//color = color * (1.0f / Uncharted2Tonemap(vec3(9.0f)));	
	// Gamma correction
	//color = pow(color, vec3(1.0f / 2.2f)); // 2.2 is gamma

	outColor = vec4(vec3(color), ALPHA);

        //outColor = vec4(color, ALPHA);
}