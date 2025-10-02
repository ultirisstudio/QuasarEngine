#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inTexCoord;

layout(set = 1, binding = 0) uniform local_uniform_object {
    mat4 model;
} object_ubo;

layout(set = 1, binding = 1) uniform samplerCube skybox; 

void main() {
    outColor = texture(skybox, inTexCoord);
}