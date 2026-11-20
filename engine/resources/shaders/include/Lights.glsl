struct PointLight
{
    vec4 PositionRange;
    vec4 Color;
};

struct SpotLight
{
    vec4 PositionRange;
    vec4 DirectionOuter;
    vec4 ColorInner;
};

layout(std140, binding = 3) uniform Lights
{
    vec3 u_DirectionalDirection;
    float u_DirectionalAmbient;
    vec3 u_DirectionalColor;
    int u_PointLightCount;
    int u_SpotLightCount;
    mat4 u_ShadowViewProjection;
    vec4 u_Shadow;
    PointLight u_PointLights[32];
    SpotLight u_SpotLights[32];
};

layout(binding = 15) uniform sampler2DShadow u_ShadowMap;

float Attenuate(vec3 toLight, float range)
{
    float distanceSquared = max(dot(toLight, toLight), 1e-4);
    float fade = clamp(1.0 - pow(distanceSquared / (range * range), 2.0), 0.0, 1.0);
    return fade * fade / distanceSquared;
}

float ReadShadow(vec3 position, vec3 normal)
{
    if (u_Shadow.x == 0.0)
    {
        return 1.0;
    }
    vec4 clip = u_ShadowViewProjection * vec4(position + normal * u_Shadow.z, 1.0);
    vec3 coordinates = clip.xyz / clip.w;
    if (coordinates.z > 1.0)
    {
        return 1.0;
    }
    vec2 center = coordinates.xy * 0.5 + 0.5;
    float depth = coordinates.z - u_Shadow.y;
    float lit = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            lit += texture(u_ShadowMap, vec3(center + vec2(x, y) * u_Shadow.w, depth));
        }
    }
    return lit / 9.0;
}
