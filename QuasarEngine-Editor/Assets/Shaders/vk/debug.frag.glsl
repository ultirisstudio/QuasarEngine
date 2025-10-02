#version 450 core

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 vColor;

layout(set = 0, binding = 0) uniform global_uniform_object  {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object  {
    mat4 model;
} object_ubo;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
