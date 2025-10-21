#version 450 core

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inTiling;
layout(location = 3) in vec2 inOffset;

layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object {
    int  useTexture;
    int  _pad0;
    int  _pad1;
    int  _pad2;
} object_ubo;

uniform sampler2D albedo_texture;

void main()
{
    vec2 uv = inTexCoord * inTiling + inOffset;

    vec4 tex = (object_ubo.useTexture != 0) ? texture(albedo_texture, uv)
                                            : vec4(1.0, 1.0, 1.0, 1.0);

    vec4 color = tex * inColor;

    if (color.a < 0.001)
        discard;

    outColor = color;
}
