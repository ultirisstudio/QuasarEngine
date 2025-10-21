#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inTexCoord;

layout(std140, binding = 0) uniform local_uniform_object {
    mat4 model;
} object_ubo;

uniform samplerCube skybox;

void main()
{
    outColor = texture(skybox, inTexCoord);
}