#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/ViewPosition.glsl"

layout(binding = 0) uniform sampler2D u_Depth;
uniform mat4 u_Projection;
uniform float u_Radius;
uniform float u_Bias;
uniform float u_Intensity;
uniform int u_SampleCount;
uniform int u_Scale;

layout(location = 0) out float o_Occlusion;
layout(location = 1) out float o_Depth;

const float GoldenAngle = 2.3999632;

ivec2 g_Size;

float ReadDepth(ivec2 pixel)
{
    float depth = texelFetch(u_Depth, clamp(pixel, ivec2(0), g_Size - 1), 0).r;
    return depth >= 1.0 ? NothingDrawn : GetViewDepth(depth, u_Projection);
}

vec3 GetPosition(ivec2 pixel, float depth)
{
    return GetViewPosition((vec2(pixel) + 0.5) / vec2(g_Size), depth, u_Projection);
}

// At half resolution, each pixel stands for a block of 2 x 2 pixels: it keeps the closest or the farthest of them in
// turns, so that both sides of an edge are computed
ivec2 ChoosePixel(out float depth)
{
    ivec2 first = ivec2(gl_FragCoord.xy) * u_Scale;
    bool closest = ((int(gl_FragCoord.x) + int(gl_FragCoord.y)) & 1) == 0;
    ivec2 chosen = first;
    float stored = texelFetch(u_Depth, min(first, g_Size - 1), 0).r;
    for (int index = 1; index < u_Scale * u_Scale; ++index)
    {
        ivec2 pixel = min(first + ivec2(index % u_Scale, index / u_Scale), g_Size - 1);
        float other = texelFetch(u_Depth, pixel, 0).r;
        if (closest ? other < stored : other > stored)
        {
            stored = other;
            chosen = pixel;
        }
    }
    depth = stored >= 1.0 ? NothingDrawn : GetViewDepth(stored, u_Projection);
    return chosen;
}

// The difference toward the neighbor on the same surface: the one whose depth is the closest to the center
vec3 GetSlope(ivec2 pixel, vec3 center, ivec2 step)
{
    float after = ReadDepth(pixel + step);
    float before = ReadDepth(pixel - step);
    vec3 forward = isinf(after) ? vec3(0.0) : GetPosition(pixel + step, after) - center;
    vec3 backward = isinf(before) ? vec3(0.0) : center - GetPosition(pixel - step, before);
    if (isinf(after))
    {
        return backward;
    }
    if (isinf(before))
    {
        return forward;
    }
    return abs(forward.z) < abs(backward.z) ? forward : backward;
}

vec3 GetNormal(ivec2 pixel, vec3 center)
{
    vec3 normal = cross(GetSlope(pixel, center, ivec2(1, 0)), GetSlope(pixel, center, ivec2(0, 1)));
    return dot(normal, normal) > 0.0 ? normalize(normal) : vec3(0.0, 0.0, 1.0);
}

mat3 GetBasis(vec3 normal)
{
    float angle = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    vec3 random = vec3(cos(angle), sin(angle), 0.0);
    vec3 tangent = random - normal * dot(random, normal);
    tangent = dot(tangent, tangent) > 1e-6 ? normalize(tangent) : normalize(cross(normal, vec3(0.0, 0.0, 1.0) + random));
    return mat3(tangent, cross(normal, tangent), normal);
}

void main()
{
    g_Size = textureSize(u_Depth, 0);
    float depth;
    ivec2 pixel = ChoosePixel(depth);
    o_Depth = depth;
    if (isinf(depth))
    {
        o_Occlusion = 1.0;
        return;
    }

    vec3 center = GetPosition(pixel, depth);
    mat3 basis = GetBasis(GetNormal(pixel, center));
    vec2 turn = vec2(cos(GoldenAngle), sin(GoldenAngle));
    vec2 direction = vec2(1.0, 0.0);
    float occlusion = 0.0;
    for (int index = 0; index < u_SampleCount; ++index)
    {
        float share = (float(index) + 0.5) / float(u_SampleCount);
        float height = 1.0 - share;
        vec3 kernel = vec3(direction * sqrt(1.0 - height * height), height) * mix(0.1, 1.0, share * share);
        direction = vec2(direction.x * turn.x - direction.y * turn.y, direction.x * turn.y + direction.y * turn.x);

        vec3 position = center + basis * kernel * u_Radius;
        vec4 clip = u_Projection * vec4(position, 1.0);
        vec2 sampleUv = clip.xy / clip.w * 0.5 + 0.5;
        if (any(lessThan(sampleUv, vec2(0.0))) || any(greaterThan(sampleUv, vec2(1.0))))
        {
            continue;
        }
        float surface = ReadDepth(ivec2(sampleUv * vec2(g_Size)));
        // Where nothing was drawn, the surface is infinitely far, so out of range
        float range = smoothstep(0.0, 1.0, u_Radius / max(abs(center.z - surface), 1e-5));
        occlusion += (surface >= position.z + u_Bias ? 1.0 : 0.0) * range;
    }
    o_Occlusion = pow(1.0 - occlusion / float(u_SampleCount), u_Intensity);
}
