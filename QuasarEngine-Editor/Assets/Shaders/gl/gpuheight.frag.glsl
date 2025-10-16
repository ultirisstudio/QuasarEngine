#version 450 core

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

struct PointLight { vec3 position; vec3 color; float attenuation; float power; };
struct DirLight   { vec3 direction; vec3 color; float power; };
#define NR_POINT_LIGHTS 4
#define NR_DIR_LIGHTS 4

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    int  usePointLight;
    int  useDirLight;
    PointLight pointLights[NR_POINT_LIGHTS];
    DirLight   dirLights[NR_DIR_LIGHTS];
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4  model;

    vec4  albedo;
    float roughness;
    float metallic;
    float ao;
    int   has_albedo_texture;
    int   has_normal_texture;
    int   has_roughness_texture;
    int   has_metallic_texture;
    int   has_ao_texture;

    float heightMult;
    int   uTextureScale;
} object_ubo;

uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform sampler2D metallic_texture;
uniform sampler2D ao_texture;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom + 1e-7);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k + 1e-7);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec2 uvScaled() { return inTexCoord * float(object_ubo.uTextureScale); }

vec4 getAlbedo()
{
    if (object_ubo.has_albedo_texture != 0)
        return texture(albedo_texture, uvScaled());
    return object_ubo.albedo;
}

float getRoughness()
{
    if (object_ubo.has_roughness_texture != 0) return texture(roughness_texture, uvScaled()).r;
    return object_ubo.roughness;
}

float getMetallic()
{
    if (object_ubo.has_metallic_texture != 0) return texture(metallic_texture, uvScaled()).r;
    return object_ubo.metallic;
}

float getAO()
{
    if (object_ubo.has_ao_texture != 0) return texture(ao_texture, uvScaled()).r;
    return object_ubo.ao;
}

vec3 getNormal()
{
    vec3 Ng = normalize(cross(dFdx(inWorldPos), dFdy(inWorldPos)));

    if (object_ubo.has_normal_texture == 0)
        return Ng;

    vec3 tangentNormal = texture(normal_texture, uvScaled()).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(inWorldPos);
    vec3 Q2  = dFdy(inWorldPos);
    vec2 st1 = dFdx(uvScaled());
    vec2 st2 = dFdy(uvScaled());

    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(Ng, T));
    mat3 TBN = mat3(T, B, Ng);

    return normalize(TBN * tangentNormal);
}

vec3 calcPointLight(PointLight light, vec3 V, vec3 N, vec3 F0, vec3 baseColor, float roughness, float metallic)
{
    vec3 L = normalize(light.position - inWorldPos);
    vec3 H = normalize(V + L);
    float dist = length(light.position - inWorldPos);
    float attenuation = 1.0 / ((1.0 + 0.09 * dist + 0.032 * dist * dist) * light.attenuation);
    vec3 radiance = (light.color * vec3(light.power)) * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  spec = (NDF * G * F) / max(4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 1e-7, 1e-7);
    vec3  kS   = F;
    vec3  kD   = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * baseColor / PI + spec) * radiance * NdotL;
}

vec3 calcDirLight(DirLight light, vec3 V, vec3 N, vec3 F0, vec3 baseColor, float roughness, float metallic)
{
    vec3 L = normalize(light.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = light.color * vec3(light.power);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  spec = (NDF * G * F) / max(4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 1e-7, 1e-7);
    vec3  kS   = F;
    vec3  kD   = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * baseColor / PI + spec) * radiance * NdotL;
}

void main()
{
    vec4 albedo   = getAlbedo();
    vec3 baseCol  = albedo.rgb;
    float metallic  = getMetallic();
    float roughness = clamp(getRoughness(), 0.05, 1.0);
    float ao       = getAO();

    vec3 N = getNormal();
    vec3 V = normalize(global_ubo.camera_position - inWorldPos);

    vec3 F0 = mix(vec3(0.04), baseCol, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < global_ubo.usePointLight; ++i)
        Lo += calcPointLight(global_ubo.pointLights[i], V, N, F0, baseCol, roughness, metallic);
    for (int i = 0; i < global_ubo.useDirLight; ++i)
        Lo += calcDirLight(global_ubo.dirLights[i], V, N, F0, baseCol, roughness, metallic);

    vec3 F  = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 ambientColor = vec3(0.25, 0.26, 0.29);
    vec3 diffuseAmb   = ambientColor * baseCol;
    vec3 ambient      = (kD * diffuseAmb) * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    // color = pow(color, vec3(1.0/2.2));

    outColor = vec4(color, 1.0);
}
