#version 450 core

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec4 inColor;

out vec4 outColor;

struct PointLight {
    vec3 position;
    vec3 color;
    float attenuation;
    float power;
};

struct DirLight {
    vec3 direction;
    vec3 color;
    float power;
};

#define NR_POINT_LIGHTS 4
#define NR_DIR_LIGHTS   4

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
    mat4 projection;
    vec3 camera_position;

    int  usePointLight;
    int  useDirLight;

    int  prefilterLevels;

    PointLight pointLights[NR_POINT_LIGHTS];
    DirLight   dirLights[NR_DIR_LIGHTS];
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4  model;

    vec4  albedo;
    float roughness;
    float metallic;
    float ao;

    int has_albedo_texture;
    int has_normal_texture;
    int has_roughness_texture;
    int has_metallic_texture;
    int has_ao_texture;
} object_ubo;

uniform sampler2D albedo_texture;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;
uniform sampler2D metallic_texture;
uniform sampler2D ao_texture;

uniform samplerCube irradiance_map;
uniform samplerCube prefilter_map;
uniform sampler2D brdf_lut;

const float PI = 3.14159265359;

vec4 getAlbedo()
{
    if (object_ubo.has_albedo_texture != 0)
        return texture(albedo_texture, inTexCoord);
    return object_ubo.albedo;
}

float getRoughness()
{
    if (object_ubo.has_roughness_texture != 0)
        return texture(roughness_texture, inTexCoord).r;
    return object_ubo.roughness;
}

float getMetallic()
{
    if (object_ubo.has_metallic_texture != 0)
        return texture(metallic_texture, inTexCoord).r;
    return object_ubo.metallic;
}

float getAO()
{
    if (object_ubo.has_ao_texture != 0)
        return texture(ao_texture, inTexCoord).r;
    return object_ubo.ao;
}

vec3 getNormalFromMap(vec3 N)
{
    vec3 tangentNormal = texture(normal_texture, inTexCoord).xyz * 2.0 - 1.0;

    vec3 T = normalize(inTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

vec3 getNormal()
{
    vec3 N = normalize(inNormal);
    if (object_ubo.has_normal_texture != 0) {
        N = getNormalFromMap(N);
    }
    return N;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
             * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

struct PBRContext {
    vec3 N;
    vec3 V;
    vec3 F0;
    vec3 albedo;
    float roughness;
    float metallic;
    float NdotV;
};

vec3 BRDF_PBR(PBRContext ctx, vec3 L, float NdotL, vec3 radiance)
{
    if (NdotL <= 0.0 || ctx.NdotV <= 0.0)
        return vec3(0.0);

    vec3 H = normalize(ctx.V + L);

    float NDF = DistributionGGX(ctx.N, H, ctx.roughness);
    float G   = GeometrySmith(ctx.NdotV, NdotL, ctx.roughness);
    vec3  F   = fresnelSchlick(max(dot(H, ctx.V), 0.0), ctx.F0);

    vec3 numerator = NDF * G * F;
    float denom    = 4.0 * ctx.NdotV * NdotL + 0.0001;
    vec3 specular  = numerator / denom;

    vec3 F_energy = fresnelSchlickRoughness(ctx.NdotV, ctx.F0, ctx.roughness);
    float avgF    = (F_energy.r + F_energy.g + F_energy.b) * 0.3333333;
    float energyComp = 1.0 + avgF * 0.5;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - ctx.metallic) * energyComp;

    vec3 diffuse = kD * ctx.albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

vec3 evaluatePointLight(PointLight light, PBRContext ctx, vec3 worldPos)
{
    vec3 L = normalize(light.position - worldPos);
    float distance = length(light.position - worldPos);

    float NdotL = max(dot(ctx.N, L), 0.0);
    if (NdotL <= 0.0)
        return vec3(0.0);

    float attenuation = 1.0 / ((1.0 + 0.09 * distance + 0.032 * (distance * distance)) * light.attenuation);
    vec3 radiance = (light.color * light.power) * attenuation;

    return BRDF_PBR(ctx, L, NdotL, radiance);
}

vec3 evaluateDirLight(DirLight light, PBRContext ctx)
{
    float power = max(light.power, 0.0);
    vec3  color = max(light.color, vec3(0.0));

    if (power <= 0.0)
        return vec3(0.0);

    vec3 L = normalize(light.direction);
    float NdotL = max(dot(ctx.N, L), 0.0);
    if (NdotL <= 0.0)
        return vec3(0.0);

    vec3 radiance = color * power;
    return BRDF_PBR(ctx, L, NdotL, radiance);
}

void main()
{
    vec4 albedoSample = getAlbedo();
    if (albedoSample.a < 0.5)
        discard;

    vec3  albedo    = albedoSample.rgb;
    float metallic  = getMetallic();
    float roughness = clamp(getRoughness(), 0.05, 1.0);
    float ao        = getAO();

    vec3 N = getNormal();
    vec3 V = normalize(global_ubo.camera_position - inWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    if (NdotV <= 0.0) {
        outColor = vec4(0.0);
        return;
    }

    vec3 R = reflect(-V, N);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    PBRContext ctx;
    ctx.N        = N;
    ctx.V        = V;
    ctx.F0       = F0;
    ctx.albedo   = albedo;
    ctx.roughness= roughness;
    ctx.metallic = metallic;
    ctx.NdotV    = NdotV;

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < global_ubo.usePointLight; ++i) {
        Lo += evaluatePointLight(global_ubo.pointLights[i], ctx, inWorldPos);
    }

    for (int i = 0; i < global_ubo.useDirLight; ++i) {
        Lo += evaluateDirLight(global_ubo.dirLights[i], ctx);
    }

    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 F_energy = fresnelSchlickRoughness(NdotV, F0, roughness);
    float avgF    = (F_energy.r + F_energy.g + F_energy.b) * 0.3333333;
    float energyComp = 1.0 + avgF * 0.5;
    kD *= energyComp;

    vec3 irradiance    = texture(irradiance_map, N).rgb;
    vec3 diffuseIBL    = irradiance * albedo;

    float maxLevel = max(float(global_ubo.prefilterLevels - 1), 0.0);
    vec3 prefilteredColor = textureLod(prefilter_map, R, roughness * maxLevel).rgb;
    vec2 brdfSample       = texture(brdf_lut, vec2(NdotV, roughness)).rg;
    vec3 specularIBL      = prefilteredColor * (F * brdfSample.x + brdfSample.y);

    vec3 ambient = (kD * diffuseIBL + specularIBL) * ao;
    
    vec3 result = ambient + Lo;

    result = result / (result + vec3(1.0));
    outColor = vec4(result, 1.0);
}