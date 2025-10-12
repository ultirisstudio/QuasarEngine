#version 450 core

layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vColor;
layout(location=0) out vec4 FragColor;

layout(std140, binding = 0) uniform global_uniform_object {
	mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
	mat4 model;
} object_ubo;

uniform sampler2D uTexture;

void main() {
    vec4 t = texture(uTexture, vUV);
    FragColor = t;
}
