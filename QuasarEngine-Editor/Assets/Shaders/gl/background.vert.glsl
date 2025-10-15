#version 450 core

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outWorldPos;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
} global_ubo;

void main()
{
    outWorldPos = inPosition;

    mat4 rotView = mat4(mat3(global_ubo.view));
    vec4 clipPos = global_ubo.projection * rotView * vec4(outWorldPos, 1.0);

    gl_Position = vec4(clipPos.xy, 1.0, 1.0);
}
