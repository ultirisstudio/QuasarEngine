#version 450 core

in vec4 vColor;
out vec4 FragColor;

layout(std140, binding = 0) uniform global_uniform_object  {
	mat4 view;
	mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
	mat4 model;
	float pointSize;
} object_ubo;

void main()
{
    FragColor = vColor;
}
