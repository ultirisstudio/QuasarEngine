#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/ViewPosition.glsl"

layout(binding = 0) uniform sampler2D u_Occlusion;
layout(binding = 1) uniform sampler2D u_Depth;
uniform mat4 u_InverseProjection;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(u_Depth, 0));
    vec2 uv = gl_FragCoord.xy * texel;
    float depth = GetViewPosition(u_Depth, u_InverseProjection, uv).z;
    float sum = 0.0;
    float weight = 0.0;
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            vec2 offset = vec2(x, y) * texel;
            float other = GetViewPosition(u_Depth, u_InverseProjection, uv + offset).z;
            float near = abs(other - depth) <= 0.05 * abs(depth) ? 1.0 : 0.0;
            sum += texture(u_Occlusion, uv + offset).r * near;
            weight += near;
        }
    }
    o_Color = vec4(vec3(weight > 0.0 ? sum / weight : 1.0), 1.0);
}
