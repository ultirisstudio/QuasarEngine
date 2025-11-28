#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

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
    gl_Position = global_ubo.projection * global_ubo.view * object_ubo.model * vec4(inPosition, 1.0);
    outColor = inColor;

    gl_PointSize = global_ubo.pointSize;
}
