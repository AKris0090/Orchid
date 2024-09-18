// SHADER PULLED FROM: https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
#version 450

layout(set = 0, binding = 0) uniform sampler2D bloomTex;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 downSampledColor;

layout(push_constant) uniform BloomPushConstant
{
	vec4 resolutionRadius;
} pc;

void main()
{
    vec2 srcTexelSize = 1.0 / pc.resolutionRadius.xy;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;
    
    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(bloomTex, vec2(fragUV.x - 2*x, fragUV.y + 2*y)).rgb;
    vec3 b = texture(bloomTex, vec2(fragUV.x,       fragUV.y + 2*y)).rgb;
    vec3 c = texture(bloomTex, vec2(fragUV.x + 2*x, fragUV.y + 2*y)).rgb;
    
    vec3 d = texture(bloomTex, vec2(fragUV.x - 2*x, fragUV.y)).rgb;
    vec3 e = texture(bloomTex, vec2(fragUV.x,       fragUV.y)).rgb;
    vec3 f = texture(bloomTex, vec2(fragUV.x + 2*x, fragUV.y)).rgb;
    
    vec3 g = texture(bloomTex, vec2(fragUV.x - 2*x, fragUV.y - 2*y)).rgb;
    vec3 h = texture(bloomTex, vec2(fragUV.x,       fragUV.y - 2*y)).rgb;
    vec3 i = texture(bloomTex, vec2(fragUV.x + 2*x, fragUV.y - 2*y)).rgb;
    
    vec3 j = texture(bloomTex, vec2(fragUV.x - x, fragUV.y + y)).rgb;
    vec3 k = texture(bloomTex, vec2(fragUV.x + x, fragUV.y + y)).rgb;
    vec3 l = texture(bloomTex, vec2(fragUV.x - x, fragUV.y - y)).rgb;
    vec3 m = texture(bloomTex, vec2(fragUV.x + x, fragUV.y - y)).rgb;
    
    downSampledColor.rgb = e*0.125;
    downSampledColor.rgb += (a+c+g+i)*0.03125;
    downSampledColor.rgb += (b+d+f+h)*0.0625;
    downSampledColor.rgb += (j+k+l+m)*0.125;
    downSampledColor.rgb = max(downSampledColor.rgb, 0.0001f);
    downSampledColor = vec4(downSampledColor.r, downSampledColor.g, downSampledColor.b, 1.0);
}