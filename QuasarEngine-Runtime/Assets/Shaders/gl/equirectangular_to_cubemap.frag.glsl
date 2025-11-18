#version 450 core

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 N = normalize(inWorldPos);
    vec2 uv = SampleSphericalMap(N);
    vec3 c = texture(equirectangularMap, uv).rgb;
    outColor = vec4(c, 1.0);
}
