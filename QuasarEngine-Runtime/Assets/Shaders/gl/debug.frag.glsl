#version 450 core

out vec4 FragColor;

layout(location = 0) in vec3 vColor;

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4 model;
} object_ubo;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
