#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/ViewPosition.glsl"

layout(binding = 0) uniform sampler2D u_Occlusion;
layout(binding = 1) uniform sampler2D u_LinearDepth;
uniform int u_Vertical;
uniform int u_Radius;

layout(location = 0) out float o_Occlusion;

// Averages 5 pixels along a line, without mixing surfaces at different depths
void main()
{
    ivec2 last = textureSize(u_LinearDepth, 0) - 1;
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 step = u_Vertical != 0 ? ivec2(0, 1) : ivec2(1, 0);
    float depth = texelFetch(u_LinearDepth, pixel, 0).r;
    if (isinf(depth))
    {
        o_Occlusion = 1.0;
        return;
    }

    float sum = 0.0;
    float weight = 0.0;
    for (int offset = -u_Radius; offset <= u_Radius; ++offset)
    {
        ivec2 other = clamp(pixel + step * offset, ivec2(0), last);
        float near = IsSameSurface(depth, texelFetch(u_LinearDepth, other, 0).r) ? 1.0 : 0.0;
        sum += texelFetch(u_Occlusion, other, 0).r * near;
        weight += near;
    }
    o_Occlusion = sum / weight;
}
