#version 450 core
in vec3 WorldPos;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 lightVP;
	vec3 lightPos;
	float far_plane;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
    mat4 model;
} object_ubo;

void main() {
    float dist  = length(WorldPos - global_ubo.lightPos);
    float depth = dist / max(global_ubo.far_plane, 1e-4);
    gl_FragDepth = clamp(depth, 0.0, 1.0);
}
