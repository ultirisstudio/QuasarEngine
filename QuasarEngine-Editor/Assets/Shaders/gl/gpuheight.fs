#version 450 core

layout (location = 0) in vec2 vTexCoord;
layout (location = 1) in float fHeight;
layout (location = 2) in vec3  fNormal;

layout (location = 0) out vec4 FragColor;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4 model;
    float heightMult;
    int   uTextureScale;
} object_ubo;

uniform sampler2D uTexture;

void main()
{
    vec2 uv = vTexCoord * float(object_ubo.uTextureScale);
    FragColor = vec4(texture(uTexture, uv).rgb, 1.0);
}
