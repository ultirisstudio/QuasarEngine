#version 450 core

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outWorldPos;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 projection;
    mat4 view;
} global_ubo;

void main()
{
    outWorldPos = inPosition;
    gl_Position = global_ubo.projection * global_ubo.view * vec4(outWorldPos, 1.0);
}
