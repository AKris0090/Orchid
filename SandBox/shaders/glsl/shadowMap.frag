#version 450

layout(push_constant) uniform PushConsts {
    mat4 mvp;
    mat4 model;
} pcb;

void main() {
	gl_FragDepth = gl_FragCoord.z;
}