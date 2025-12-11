#version 450 core

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 vColor;
layout(location = 2) in float vAlphaFactor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D particle_texture;

void main()
{
    vec4 tex = texture(particle_texture, vTexCoord);

    if (tex.a <= 0.001)
        discard;

    float alpha = tex.a * vAlphaFactor * vColor.a;
    if (alpha <= 0.001)
        discard;

    outColor = vec4(tex.rgb * vColor.rgb, alpha);
}
