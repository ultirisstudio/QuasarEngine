#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(binding = 0) uniform sampler2D u_Scene;
uniform float u_Exposure;
uniform int u_Tonemapping;

layout(location = 0) out vec4 o_Color;

vec3 Reinhard(vec3 color)
{
    return color / (1.0 + color);
}

vec3 Aces(vec3 color)
{
    return clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

vec3 LinearToSrgb(vec3 color)
{
    return mix(color * 12.92, 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055, greaterThan(color, vec3(0.0031308)));
}

void main()
{
    vec3 color = max(texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0).rgb * u_Exposure, vec3(0.0));
    if (u_Tonemapping == 1)
    {
        color = Reinhard(color);
    }
    else if (u_Tonemapping == 2)
    {
        color = Aces(color);
    }
    o_Color = vec4(LinearToSrgb(clamp(color, 0.0, 1.0)), 1.0);
}
