#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out uint outEntityID;

in vec2 fTextureCoordinates;
in vec3 fWorldPos;
in vec3 fNormal;
flat in float fTextureIndice;

uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D uBrdfLUT;

// material parameters
struct Material
{
    bool use_albedo_texture;
    bool use_normal_texture;
    bool use_metallic_texture;
    bool use_roughness_texture;
    bool use_ao_texture;

    vec3 albedoColor;
    float metallic;
    float roughness;
    float ao;

    sampler2DArray albedoMap;
    sampler2DArray normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};

//Point light
struct PointLight {    
    vec3 position;
    vec3 color;

    float attenuation;
    float power;
};
#define NR_POINT_LIGHTS 4
uniform int uUsePointLight;
uniform PointLight uPointLights[NR_POINT_LIGHTS];

//Directional light
struct DirLight {    
    vec3 direction;
    vec3 color;
    
    float power;
};
#define NR_DIR_LIGHTS 4
uniform int uUseDirLight;
uniform DirLight uDirLights[NR_DIR_LIGHTS];

uniform Material uMaterial;

uniform vec3 uCameraPosition;

uniform float uAmbiantLight;

uniform uint uEntity;

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    int index = int(fTextureIndice) - 1;
    vec3 tangentNormal = texture(uMaterial.normalMap, vec3(fTextureCoordinates, index)).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fWorldPos);
    vec3 Q2  = dFdy(fWorldPos);
    vec2 st1 = dFdx(fTextureCoordinates);
    vec2 st2 = dFdy(fTextureCoordinates);

    vec3 N   = normalize(fNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal).xyz;
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  
// ----------------------------------------------------------------------------
vec3 calculatePointLightReflectance(PointLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
{
    // calculate per-light radiance
    vec3 L = normalize(light.position - fWorldPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position - fWorldPos);
    float attenuation = 1.0 / ((1.0 + 0.09 * distance + 0.032 * (distance * distance)) * light.attenuation); //(light.constant + light.linear * distance + light.quadratic * (distance * distance)) (distance * distance)
    vec3 radiance = (light.color * vec3(light.power)) * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);

    return ((kD * albedo / PI + specular) * radiance * NdotL);
}
// ----------------------------------------------------------------------------
vec3 calculateDirLightReflectance(DirLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
{
    // calculate per-light radiance
    vec3 L = normalize(light.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = light.color * vec3(light.power);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}
// ----------------------------------------------------------------------------
void main()
{
    vec3 albedo = uMaterial.albedoColor;
    float metallic = uMaterial.metallic;
    float roughness = uMaterial.roughness;
    float ao = uMaterial.ao;
    vec3 N = normalize(fNormal).xyz;

    int index = int(fTextureIndice) - 1;

    if (uMaterial.use_albedo_texture)
    {
        //albedo = pow(texture(uMaterial.albedoMap, vec3(fTextureCoordinates, index)).rgb, vec3(2.2));
        vec4 temp = texture(uMaterial.albedoMap, vec3(fTextureCoordinates, index));

        if (temp.a < 0.5)
		{
			discard;
		}

        albedo = temp.rgb;
    }

    if (uMaterial.use_normal_texture)
    {
        N = getNormalFromMap();
    }

    if (uMaterial.use_metallic_texture)
    {
        metallic = texture(uMaterial.metallicMap, fTextureCoordinates).r;
    }

    if (uMaterial.use_roughness_texture)
    {
        roughness = texture(uMaterial.roughnessMap, fTextureCoordinates).r;
    }

    if (uMaterial.use_ao_texture)
    {
        ao = texture(uMaterial.aoMap, fTextureCoordinates).r;
    }

    vec3 V = normalize(uCameraPosition - fWorldPos);
    vec3 R = reflect(-V, N); 
  
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < uUsePointLight; ++i)
    {
        Lo += calculatePointLightReflectance(uPointLights[i], V, N, F0, albedo, roughness, metallic);
    }
    for(int i = 0; i < uUseDirLight; ++i)
    {
        Lo += calculateDirLightReflectance(uDirLights[i], V, N, F0, albedo, roughness, metallic);
    }

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F; 
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(uIrradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(uPrefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(uBrdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 result = ambient + Lo;

    // HDR tonemapping
    result = result / (result + vec3(1.0));
    // gamma correct
    //result = pow(result, vec3(1.0/2.2)); 

    color = vec4(result, 1.0);
    //color = vec4(irradiance, 1.0);

    outEntityID = uEntity;
}