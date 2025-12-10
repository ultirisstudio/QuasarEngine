#version 450 core

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 1) uniform local_uniform_object {
    mat4 model;
    vec4 color;
    float life;
    float maxLife;
} object_ubo;

uniform sampler2D particle_texture;

void main()
{
    vec4 tex = texture(particle_texture, inTexCoord);

    float life01 = clamp(object_ubo.life / max(object_ubo.maxLife, 0.0001), 0.0, 1.0);
    float fade = life01;

    float alpha = tex.a * object_ubo.color.a * fade;

    if (alpha < 0.01)
        discard;

    vec3 rgb = tex.rgb * object_ubo.color.rgb;
    outColor = vec4(rgb, alpha);
}
