#version 450 core
layout (location=0) in vec3 inPosition;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 lightVP;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
    mat4 model;
} object_ubo;

void main() {
    gl_Position = global_ubo.lightVP * object_ubo.model * vec4(inPosition, 1.0);
}
