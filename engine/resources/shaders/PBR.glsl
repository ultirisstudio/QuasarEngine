#type vertex
#version 450 core
#extension GL_ARB_shader_draw_parameters : require
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Tangent;

#include "include/Common.glsl"

layout(location = 0) out vec3 v_Position;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out vec4 v_Tangent;

void main()
{
    ObjectData object = u_Objects[gl_BaseInstanceARB + gl_InstanceID];
    vec4 position = object.Model * vec4(a_Position, 1.0);
    mat3 model = mat3(object.Model);
    v_Position = position.xyz;
    v_Normal = object.NormalMatrix * a_Normal;
    v_TexCoord = a_TexCoord;
    v_Tangent = vec4(model * a_Tangent.xyz, determinant(model) < 0.0 ? -a_Tangent.w : a_Tangent.w);
    gl_Position = u_ViewProjection * position;
}

#type fragment
#version 450 core
layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec4 v_Tangent;

#include "include/Common.glsl"

layout(std140, binding = 1) uniform Material
{
    vec4 u_Color;
    vec3 u_Emissive;
    float u_EmissiveIntensity;
    float u_Metallic;
    float u_Roughness;
    float u_NormalScale;
    float u_OcclusionStrength;
};

#include "include/Lights.glsl"
#include "include/BRDF.glsl"

layout(binding = 0) uniform sampler2D u_Texture;
layout(binding = 1) uniform sampler2D u_NormalMap;
layout(binding = 2) uniform sampler2D u_MetallicRoughnessMap;
layout(binding = 3) uniform sampler2D u_OcclusionMap;
layout(binding = 4) uniform sampler2D u_EmissiveMap;

layout(location = 0) out vec4 o_Color;

const float MinPerceptualRoughness = 0.045;
const float DielectricReflectance = 0.04;
const float SpecularAntiAliasingVariance = 0.15;
const float SpecularAntiAliasingThreshold = 0.2;

struct Surface
{
    vec3 Normal;
    vec3 ToCamera;
    float NormalDotView;
    vec3 DiffuseColor;
    vec3 Reflectance;
    float PerceptualRoughness;
    float Roughness;
};

vec3 GetNormal(vec3 geometric, vec4 tangent)
{
    vec3 mapped = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
    mapped.xy *= u_NormalScale;
    vec3 bitangent = cross(geometric, tangent.xyz) * tangent.w;
    return normalize(tangent.xyz * mapped.x + bitangent * mapped.y + geometric * mapped.z);
}

float FilterRoughness(vec3 normal, float perceptualRoughness)
{
    vec3 alongX = dFdx(normal);
    vec3 alongY = dFdy(normal);
    float variance = SpecularAntiAliasingVariance * (dot(alongX, alongX) + dot(alongY, alongY));
    float roughness = perceptualRoughness * perceptualRoughness;
    float kernel = min(2.0 * variance, SpecularAntiAliasingThreshold);
    return sqrt(sqrt(clamp(roughness * roughness + kernel, 0.0, 1.0)));
}

vec3 Shade(Surface surface, vec3 toLight, vec3 radiance)
{
    float normalDotLight = dot(surface.Normal, toLight);
    if (normalDotLight <= 0.0)
    {
        return vec3(0.0);
    }
    vec3 halfway = normalize(toLight + surface.ToCamera);
    float normalDotHalf = clamp(dot(surface.Normal, halfway), 0.0, 1.0);
    float lightDotHalf = clamp(dot(toLight, halfway), 0.0, 1.0);
    float distribution = DistributionGGX(normalDotHalf, surface.Roughness);
    float visibility = VisibilitySmithGGX(surface.NormalDotView, normalDotLight, surface.Roughness);
    vec3 specular = distribution * visibility * FresnelSchlick(surface.Reflectance, lightDotHalf);
    return (surface.DiffuseColor + Pi * specular) * radiance * normalDotLight;
}

void main()
{
    vec4 base = texture(u_Texture, v_TexCoord) * vec4(SrgbToLinear(u_Color.rgb), u_Color.a);
    vec2 metallicRoughness = texture(u_MetallicRoughnessMap, v_TexCoord).bg;
    float metallic = clamp(u_Metallic * metallicRoughness.x, 0.0, 1.0);

    float facing = gl_FrontFacing ? 1.0 : -1.0;
    vec3 geometric = normalize(v_Normal) * facing;
    vec4 tangent = v_Tangent * facing;

    Surface surface;
    surface.Normal = GetNormal(geometric, tangent);
    surface.ToCamera = normalize(u_CameraPosition.xyz - v_Position);
    surface.NormalDotView = max(dot(surface.Normal, surface.ToCamera), 1e-4);
    surface.DiffuseColor = base.rgb * (1.0 - metallic);
    surface.Reflectance = mix(vec3(DielectricReflectance), base.rgb, metallic);
    surface.PerceptualRoughness = FilterRoughness(geometric, clamp(u_Roughness * metallicRoughness.y, MinPerceptualRoughness, 1.0));
    surface.Roughness = surface.PerceptualRoughness * surface.PerceptualRoughness;

    vec3 color = vec3(0.0);
    vec3 toSun = -u_DirectionalDirection;
    if (dot(surface.Normal, toSun) > 0.0)
    {
        color += Shade(surface, toSun, u_DirectionalColor * ReadShadow(v_Position, geometric));
    }

    for (int index = 0; index < u_PointLightCount; ++index)
    {
        PointLight light = u_PointLights[index];
        vec3 toLight = light.PositionRange.xyz - v_Position;
        if (IsInRange(toLight, light.PositionRange.w))
        {
            color += Shade(surface, normalize(toLight), GetPointLightRadiance(light, toLight));
        }
    }

    for (int index = 0; index < u_SpotLightCount; ++index)
    {
        SpotLight light = u_SpotLights[index];
        vec3 toLight = light.PositionRange.xyz - v_Position;
        if (IsInRange(toLight, light.PositionRange.w))
        {
            vec3 direction = normalize(toLight);
            color += Shade(surface, direction, GetSpotLightRadiance(light, toLight, direction));
        }
    }

    float screenOcclusion = ReadAmbientOcclusion();
    float occlusion = screenOcclusion * mix(1.0, texture(u_OcclusionMap, v_TexCoord).r, u_OcclusionStrength);
    vec3 ambient = u_DirectionalColor * u_DirectionalAmbient;
    vec3 environment = EnvironmentBRDF(surface.Reflectance, surface.PerceptualRoughness, surface.NormalDotView);
    vec3 indirect = surface.DiffuseColor * (1.0 - environment) * occlusion +
                    environment * GetSpecularOcclusion(surface.NormalDotView, occlusion, surface.Roughness);
    vec3 emission = SrgbToLinear(u_Emissive) * u_EmissiveIntensity * texture(u_EmissiveMap, v_TexCoord).rgb;

    o_Color = vec4(color * GetDirectOcclusion(screenOcclusion) + ambient * indirect + emission, base.a);
}
