#version 450 core

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;

layout(std140, binding = 0) uniform global_uniform_object {
	mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
	mat4 model;
} object_ubo;

void main()
{
    vUV    = inTexCoord;
    vColor = inColor;
    gl_Position = global_ubo.projection * vec4(inPosition, 0.0, 1.0);
}
