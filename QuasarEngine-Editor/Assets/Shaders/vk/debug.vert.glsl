#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 vColor;

layout(set = 0, binding = 0) uniform global_uniform_object  {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object  {
    mat4 model;
} object_ubo;

void main()
{
    gl_Position = global_ubo.projection * global_ubo.view * object_ubo.model * vec4(inPosition, 1.0);
    vColor = inColor;
}
