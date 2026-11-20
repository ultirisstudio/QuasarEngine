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

layout(std140, binding = 3) uniform Light
{
    vec3 u_LightDirection;
    float u_LightAmbient;
    vec3 u_LightColor;
};

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 0) out vec4 o_Color;

vec3 SrgbToLinear(vec3 color)
{
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), greaterThan(color, vec3(0.04045)));
}

void main()
{
    vec4 base = texture(u_Texture, v_TexCoord) * vec4(SrgbToLinear(u_Color.rgb), u_Color.a);
    vec3 normal = normalize(v_Normal);
    vec3 toLight = -u_LightDirection;
    vec3 toCamera = normalize(u_CameraPosition.xyz - v_Position);

    float diffuse = max(dot(normal, toLight), 0.0);
    float specular = 0.0;
    if (diffuse > 0.0)
    {
        specular = u_Specular * pow(max(dot(normal, normalize(toLight + toCamera)), 0.0), u_Shininess);
    }

    o_Color = vec4(u_LightColor * (base.rgb * (u_LightAmbient + diffuse) + specular), base.a);
}
