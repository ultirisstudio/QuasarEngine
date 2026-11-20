#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    mat4 u_Projection;
    vec4 u_CameraPosition;
};

layout(std140, binding = 2) uniform Object
{
    mat4 u_Model;
};

layout(location = 0) out vec3 v_Position;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;

void main()
{
    vec4 position = u_Model * vec4(a_Position, 1.0);
    v_Position = position.xyz;
    v_Normal = transpose(inverse(mat3(u_Model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * position;
}

#type fragment
#version 450 core
layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    mat4 u_Projection;
    vec4 u_CameraPosition;
};

layout(std140, binding = 1) uniform Material
{
    vec4 u_Color;
    float u_Specular;
    float u_Shininess;
};

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

layout(binding = 0) uniform sampler2D u_Texture;
layout(binding = 15) uniform sampler2DShadow u_ShadowMap;

layout(location = 0) out vec4 o_Color;

vec3 SrgbToLinear(vec3 color)
{
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), greaterThan(color, vec3(0.04045)));
}

vec3 Shade(vec3 base, vec3 normal, vec3 toCamera, vec3 toLight, vec3 radiance)
{
    float diffuse = max(dot(normal, toLight), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0)
    {
        specular = u_Specular * pow(max(dot(normal, normalize(toLight + toCamera)), 0.0), u_Shininess);
    }
    return radiance * (base * diffuse + specular);
}

float Attenuate(vec3 toLight, float range)
{
    float distanceSquared = max(dot(toLight, toLight), 1e-4);
    float fade = clamp(1.0 - pow(distanceSquared / (range * range), 2.0), 0.0, 1.0);
    return fade * fade / distanceSquared;
}

float ReadShadow(vec3 normal)
{
    if (u_Shadow.x == 0.0)
    {
        return 1.0;
    }
    vec4 clip = u_ShadowViewProjection * vec4(v_Position + normal * u_Shadow.z, 1.0);
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

void main()
{
    vec4 base = texture(u_Texture, v_TexCoord) * vec4(SrgbToLinear(u_Color.rgb), u_Color.a);
    vec3 normal = normalize(v_Normal);
    vec3 toCamera = normalize(u_CameraPosition.xyz - v_Position);

    vec3 color = u_DirectionalColor * base.rgb * u_DirectionalAmbient;
    color += Shade(base.rgb, normal, toCamera, -u_DirectionalDirection, u_DirectionalColor * ReadShadow(normal));

    for (int index = 0; index < u_PointLightCount; ++index)
    {
        PointLight light = u_PointLights[index];
        vec3 toLight = light.PositionRange.xyz - v_Position;
        float attenuation = Attenuate(toLight, light.PositionRange.w);
        color += Shade(base.rgb, normal, toCamera, normalize(toLight), light.Color.rgb * attenuation);
    }

    for (int index = 0; index < u_SpotLightCount; ++index)
    {
        SpotLight light = u_SpotLights[index];
        vec3 toLight = light.PositionRange.xyz - v_Position;
        vec3 direction = normalize(toLight);
        float cone = clamp((dot(-direction, light.DirectionOuter.xyz) - light.DirectionOuter.w) / (light.ColorInner.w - light.DirectionOuter.w), 0.0, 1.0);
        float attenuation = Attenuate(toLight, light.PositionRange.w) * cone * cone;
        color += Shade(base.rgb, normal, toCamera, direction, light.ColorInner.rgb * attenuation);
    }

    o_Color = vec4(color, base.a);
}
