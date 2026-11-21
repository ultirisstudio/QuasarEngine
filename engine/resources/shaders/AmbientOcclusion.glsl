#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/ViewPosition.glsl"

layout(binding = 0) uniform sampler2D u_Depth;
uniform mat4 u_Projection;
uniform mat4 u_InverseProjection;
uniform float u_Radius;
uniform float u_Bias;
uniform float u_Intensity;
uniform int u_SampleCount;

layout(location = 0) out vec4 o_Color;

vec3 GetPosition(vec2 uv)
{
    return GetViewPosition(u_Depth, u_InverseProjection, uv);
}

vec3 GetNormal(vec2 uv, vec3 center, vec2 texel)
{
    vec3 right = GetPosition(uv + vec2(texel.x, 0.0)) - center;
    vec3 left = center - GetPosition(uv - vec2(texel.x, 0.0));
    vec3 up = GetPosition(uv + vec2(0.0, texel.y)) - center;
    vec3 down = center - GetPosition(uv - vec2(0.0, texel.y));
    vec3 alongX = abs(right.z) < abs(left.z) ? right : left;
    vec3 alongY = abs(up.z) < abs(down.z) ? up : down;
    return normalize(cross(alongX, alongY));
}

vec3 GetKernel(int index, int count)
{
    float share = (float(index) + 0.5) / float(count);
    float height = 1.0 - share;
    float spread = sqrt(1.0 - height * height);
    float angle = float(index) * 2.3999632;
    return vec3(cos(angle) * spread, sin(angle) * spread, height) * mix(0.1, 1.0, share * share);
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
    vec2 texel = 1.0 / vec2(textureSize(u_Depth, 0));
    vec2 uv = gl_FragCoord.xy * texel;
    if (texture(u_Depth, uv).r >= 1.0)
    {
        o_Color = vec4(1.0);
        return;
    }

    vec3 center = GetPosition(uv);
    mat3 basis = GetBasis(GetNormal(uv, center, texel));
    float occlusion = 0.0;
    for (int index = 0; index < u_SampleCount; ++index)
    {
        vec3 position = center + basis * GetKernel(index, u_SampleCount) * u_Radius;
        vec4 clip = u_Projection * vec4(position, 1.0);
        vec2 sampleUv = clip.xy / clip.w * 0.5 + 0.5;
        if (any(lessThan(sampleUv, vec2(0.0))) || any(greaterThan(sampleUv, vec2(1.0))))
        {
            continue;
        }
        float surface = GetPosition(sampleUv).z;
        float range = smoothstep(0.0, 1.0, u_Radius / max(abs(center.z - surface), 1e-5));
        occlusion += (surface >= position.z + u_Bias ? 1.0 : 0.0) * range;
    }
    o_Color = vec4(vec3(pow(1.0 - occlusion / float(u_SampleCount), u_Intensity)), 1.0);
}
