#version 450

layout(set = 0, binding = 0) uniform sampler2D bloomTex;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 upSampledColor;

layout(push_constant) uniform BloomPushConstant
{
	vec4 resolutionRadius;
} pc;

void main()
{
    float x = pc.resolutionRadius.z;
    float y = pc.resolutionRadius.z;
    
    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(bloomTex, vec2(fragUV.x - x, fragUV.y + y)).rgb;
    vec3 b = texture(bloomTex, vec2(fragUV.x,     fragUV.y + y)).rgb;
    vec3 c = texture(bloomTex, vec2(fragUV.x + x, fragUV.y + y)).rgb;
    
    vec3 d = texture(bloomTex, vec2(fragUV.x - x, fragUV.y)).rgb;
    vec3 e = texture(bloomTex, vec2(fragUV.x,     fragUV.y)).rgb;
    vec3 f = texture(bloomTex, vec2(fragUV.x + x, fragUV.y)).rgb;
    
    vec3 g = texture(bloomTex, vec2(fragUV.x - x, fragUV.y - y)).rgb;
    vec3 h = texture(bloomTex, vec2(fragUV.x,     fragUV.y - y)).rgb;
    vec3 i = texture(bloomTex, vec2(fragUV.x + x, fragUV.y - y)).rgb;
    
    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    upSampledColor.rgb = e*4.0;
    upSampledColor.rgb += (b+d+f+h)*2.0;
    upSampledColor.rgb += (a+c+g+i);
    upSampledColor.rgb *= 1.0 / 16.0;
    
    upSampledColor = vec4(upSampledColor.r, upSampledColor.g, upSampledColor.b, 1.0);
}