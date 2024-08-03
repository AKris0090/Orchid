#version 450

layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec2 texCoord;

void main() {
	gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
	texCoord = vec2(inPosition.z, inPosition.w);
}