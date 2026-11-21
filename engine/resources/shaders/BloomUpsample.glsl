#type vertex
#version 450 core
#include "include/FullscreenTriangle.glsl"

#type fragment
#version 450 core
#include "include/Filters.glsl"

layout(binding = 0) uniform sampler2D u_Source;
uniform vec2 u_TargetTexel;
uniform float u_Radius;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec2 offset = u_Radius / vec2(textureSize(u_Source, 0));
    o_Color = vec4(SampleTent(u_Source, gl_FragCoord.xy * u_TargetTexel, offset), 1.0);
}
