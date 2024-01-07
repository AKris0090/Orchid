#version 450

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragViewVec;
layout(location = 3) in vec3 fragLightVec;
layout(location = 4) in vec3 fragNormal;
layout(location = 5) in vec4 fragTangent;

layout(location = 0) out vec4 outColor;

layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0f;

void main() {
    vec4 color = texture(colorSampler, fragTexCoord) * vec4(fragColor, 1.0);

    if (ALPHA_MASK) {
        if (color.a < ALPHA_MASK_CUTOFF) {
            discard;
        }
    }

    vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    vec3 B = cross(fragNormal, fragTangent.xyz) * fragTangent.w;
    mat3 TBN = mat3(T, B, N);
    N = TBN * normalize(texture(normalSampler, fragTexCoord).xyz * 2.0 - vec3(1.0));

    const float ambient = 0.1;
    vec3 L = normalize(fragLightVec);
    vec3 V = normalize(fragViewVec);
    vec3 R = reflect(-L, N);
    vec3 diffuse = max(dot(N, L), ambient).rrr;
    float specular = pow(max(dot(R, V), 0.0), 32.0);
    outColor = vec4(diffuse * color.rgb + specular, color.a);
}