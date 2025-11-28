#version 450 core

layout(location = 0) in vec4 inColor;

out vec4 FragColor;

layout(std140, binding = 0) uniform global_uniform_object  {
	mat4 view;
	mat4 projection;
	float pointSize;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
	mat4 model;
} object_ubo;

void main()
{
    FragColor = inColor;
}
