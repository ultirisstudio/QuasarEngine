#version 450 core

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float vLife;
layout(location = 3) in float vMaxLife;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D particle_texture;

void main()
{
    vec4 texColor = texture(particle_texture, vTexCoord);

    if (texColor.a <= 0.001)
        discard;

    outColor = texColor * vColor;
}
