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
    vec4 u_AmbientOcclusion;
    PointLight u_PointLights[32];
    SpotLight u_SpotLights[32];
};

layout(binding = 14) uniform sampler2D u_AmbientOcclusionMap;
layout(binding = 15) uniform sampler2DShadow u_ShadowMap;

float ReadAmbientOcclusion()
{
    return u_AmbientOcclusion.x == 0.0 ? 1.0 : texelFetch(u_AmbientOcclusionMap, ivec2(gl_FragCoord.xy), 0).r;
}

float GetDirectOcclusion(float occlusion)
{
    return mix(1.0, occlusion, u_AmbientOcclusion.y);
}

float Attenuate(vec3 toLight, float range)
{
    float distanceSquared = max(dot(toLight, toLight), 1e-4);
    float fade = clamp(1.0 - pow(distanceSquared / (range * range), 2.0), 0.0, 1.0);
    return fade * fade / distanceSquared;
}

bool IsInRange(vec3 toLight, float range)
{
    return dot(toLight, toLight) < range * range;
}

vec3 GetPointLightRadiance(PointLight light, vec3 toLight)
{
    return light.Color.rgb * Attenuate(toLight, light.PositionRange.w);
}

vec3 GetSpotLightRadiance(SpotLight light, vec3 toLight, vec3 direction)
{
    float cone = clamp((dot(-direction, light.DirectionOuter.xyz) - light.DirectionOuter.w) / (light.ColorInner.w - light.DirectionOuter.w), 0.0, 1.0);
    return light.ColorInner.rgb * (Attenuate(toLight, light.PositionRange.w) * cone * cone);
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
