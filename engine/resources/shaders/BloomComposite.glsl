#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/Filters.glsl"

layout(binding = 0) uniform sampler2D u_Source;
layout(binding = 1) uniform sampler2D u_Bloom;
uniform vec2 u_TargetTexel;
uniform float u_Radius;
uniform float u_Intensity;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec4 source = texelFetch(u_Source, ivec2(gl_FragCoord.xy), 0);
    vec2 offset = u_Radius / vec2(textureSize(u_Bloom, 0));
    o_Color = vec4(source.rgb + SampleTent(u_Bloom, gl_FragCoord.xy * u_TargetTexel, offset) * u_Intensity, source.a);
}
