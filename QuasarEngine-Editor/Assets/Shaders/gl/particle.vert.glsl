#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
    mat4 model;
    vec4 color;
    float life;
    float maxLife;
} object_ubo;

void main()
{
    outTexCoord = inTexCoord;

    vec4 worldPos = object_ubo.model * vec4(inPosition, 1.0);
    gl_Position = global_ubo.projection * global_ubo.view * worldPos;
}
