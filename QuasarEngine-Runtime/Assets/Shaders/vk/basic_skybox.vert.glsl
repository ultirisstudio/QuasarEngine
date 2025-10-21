#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outTexCoord;

layout(set = 0, binding = 0) uniform global_uniform_object {
	mat4 view;
	mat4 projection;
} global_ubo;

void main() {
	outTexCoord = inPosition;
	mat4 rotView = mat4(mat3(global_ubo.view));
	vec4 pos = global_ubo.projection * rotView * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;
}