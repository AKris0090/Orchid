#version 450 // SHADER PULLED FROM: https://github.com/SaschaWillems/Vulkan/blob/4d2117d3d9bc27910140c1fe668003a248a2d36a/shaders/glsl/pbribl/genbrdflut.vert

layout (location = 0) out vec2 outUV;

void main()
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}