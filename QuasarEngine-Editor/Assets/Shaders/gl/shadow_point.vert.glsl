#version 450 core

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 lightVP;
	vec3 lightPos;
	float far_plane;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
    mat4 model;
} object_ubo;

out vec3 WorldPos;

void main() {
    vec4 wp = object_ubo.model * vec4(inPosition, 1.0);
    WorldPos = wp.xyz;
    gl_Position = global_ubo.lightVP * wp;
}
