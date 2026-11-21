#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
layout(binding = 0) uniform sampler2D u_Source;
uniform vec2 u_TargetTexel;
uniform int u_Prefilter;
uniform vec4 u_Threshold;

layout(location = 0) out vec4 o_Color;

vec3 Fetch(vec2 uv)
{
    return max(texture(u_Source, uv).rgb, vec3(0.0));
}

vec3 KarisAverage(vec3 a, vec3 b, vec3 c, vec3 d)
{
    const vec3 luma = vec3(0.2126, 0.7152, 0.0722);
    vec4 weights = 1.0 / (1.0 + vec4(dot(a, luma), dot(b, luma), dot(c, luma), dot(d, luma)));
    return (a * weights.x + b * weights.y + c * weights.z + d * weights.w) / (weights.x + weights.y + weights.z + weights.w);
}

vec3 KeepBright(vec3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    float soft = clamp(brightness - u_Threshold.y, 0.0, u_Threshold.z);
    soft = soft * soft * u_Threshold.w;
    return color * max(soft, brightness - u_Threshold.x) / max(brightness, 1e-5);
}

void main()
{
    vec2 uv = gl_FragCoord.xy * u_TargetTexel;
    vec2 texel = 1.0 / vec2(textureSize(u_Source, 0));
    vec3 a = Fetch(uv + texel * vec2(-2.0, 2.0));
    vec3 b = Fetch(uv + texel * vec2(0.0, 2.0));
    vec3 c = Fetch(uv + texel * vec2(2.0, 2.0));
    vec3 d = Fetch(uv + texel * vec2(-2.0, 0.0));
    vec3 e = Fetch(uv);
    vec3 f = Fetch(uv + texel * vec2(2.0, 0.0));
    vec3 g = Fetch(uv + texel * vec2(-2.0, -2.0));
    vec3 h = Fetch(uv + texel * vec2(0.0, -2.0));
    vec3 i = Fetch(uv + texel * vec2(2.0, -2.0));
    vec3 j = Fetch(uv + texel * vec2(-1.0, 1.0));
    vec3 k = Fetch(uv + texel * vec2(1.0, 1.0));
    vec3 l = Fetch(uv + texel * vec2(-1.0, -1.0));
    vec3 m = Fetch(uv + texel * vec2(1.0, -1.0));

    vec3 color;
    if (u_Prefilter != 0)
    {
        color = KarisAverage(j, k, l, m) * 0.5;
        color += (KarisAverage(a, b, d, e) + KarisAverage(b, c, e, f) + KarisAverage(d, e, g, h) + KarisAverage(e, f, h, i)) * 0.125;
        color = KeepBright(color);
    }
    else
    {
        color = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    }
    o_Color = vec4(color, 1.0);
}
