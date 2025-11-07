#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inTiling;
layout(location = 4) in vec2 inOffset;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec2 outTiling;
layout(location = 3) out vec2 outOffset;

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
} global_ubo;

void main()
{
    gl_Position = global_ubo.projection * global_ubo.view * vec4(inPosition, 1.0);
    outColor    = inColor;
    outTexCoord = inTexCoord;
    outTiling   = inTiling;
    outOffset   = inOffset;
}
