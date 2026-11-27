#type vertex
#version 450 core
#extension GL_ARB_shader_draw_parameters : require
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

#include "include/Common.glsl"

layout(location = 0) out vec3 v_Position;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;

void main()
{
    ObjectData object = u_Objects[gl_BaseInstanceARB + gl_InstanceID];
    vec4 position = object.Model * vec4(a_Position, 1.0);
    v_Position = position.xyz;
    v_Normal = object.NormalMatrix * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * position;
}

#type fragment
#version 450 core
layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

#include "include/Common.glsl"

layout(std140, binding = 1) uniform Material
{
    vec4 u_Color;
    float u_Specular;
    float u_Shininess;
};

#include "include/Lights.glsl"

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 0) out vec4 o_Color;

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

void main()
{
    vec4 base = texture(u_Texture, v_TexCoord) * vec4(SrgbToLinear(u_Color.rgb), u_Color.a);
    vec3 normal = normalize(v_Normal);
    vec3 toCamera = normalize(u_CameraPosition.xyz - v_Position);

    vec3 color = Shade(base.rgb, normal, toCamera, -u_DirectionalDirection, u_DirectionalColor * ReadShadow(v_Position, normal));

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

    float occlusion = ReadAmbientOcclusion();
    color = color * GetDirectOcclusion(occlusion) + u_DirectionalColor * base.rgb * u_DirectionalAmbient * occlusion;

    o_Color = vec4(color, base.a);
}
