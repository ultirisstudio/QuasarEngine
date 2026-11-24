#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/ViewPosition.glsl"

layout(binding = 0) uniform sampler2D u_Occlusion;
layout(binding = 1) uniform sampler2D u_LinearDepth;
layout(binding = 2) uniform sampler2D u_Depth;
uniform mat4 u_Projection;

layout(location = 0) out float o_Occlusion;

// Mixes the 4 closest pixels of the half resolution image that are on the same surface, so that edges stay sharp
void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    float stored = texelFetch(u_Depth, pixel, 0).r;
    if (stored >= 1.0)
    {
        o_Occlusion = 1.0;
        return;
    }
    float depth = GetViewDepth(stored, u_Projection);

    ivec2 last = textureSize(u_Occlusion, 0) - 1;
    vec2 position = (vec2(pixel) + 0.5) * 0.5 - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 fraction = position - vec2(base);
    float sum = 0.0;
    float weight = 0.0;
    float closest = NothingDrawn;
    float fallback = 1.0;
    for (int y = 0; y <= 1; ++y)
    {
        for (int x = 0; x <= 1; ++x)
        {
            ivec2 other = clamp(base + ivec2(x, y), ivec2(0), last);
            float otherDepth = texelFetch(u_LinearDepth, other, 0).r;
            float occlusion = texelFetch(u_Occlusion, other, 0).r;
            float bilinear = (x == 0 ? 1.0 - fraction.x : fraction.x) * (y == 0 ? 1.0 - fraction.y : fraction.y);
            if (IsSameSurface(depth, otherDepth))
            {
                sum += occlusion * bilinear;
                weight += bilinear;
            }
            float difference = abs(otherDepth - depth);
            if (difference < closest)
            {
                closest = difference;
                fallback = occlusion;
            }
        }
    }
    o_Occlusion = weight > 1e-4 ? sum / weight : fallback;
}
