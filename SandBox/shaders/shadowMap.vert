#version 450

layout(push_constant) uniform pushConstant {
    mat4 vp;
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
 
void main()
{
	gl_Position =  pc.vp * pc.model * vec4(inPosition, 1.0);
}