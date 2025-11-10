#version 450 core

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;

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
#define NR_DIR_LIGHTS 4

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
	mat4 projection;
	vec3 camera_position;
	
	int usePointLight;
	int useDirLight;

<<<<<<< HEAD
    int  usePointLight;
    int  useDirLight;

    int  prefilterLevels;

    PointLight pointLights[NR_POINT_LIGHTS];
    DirLight   dirLights[NR_DIR_LIGHTS];
=======
    int prefilterLevels;
	
	PointLight pointLights[NR_POINT_LIGHTS];
	DirLight dirLights[NR_DIR_LIGHTS];
>>>>>>> parent of 6e4f8d6 (Update)
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4 model;
	
    vec4 albedo;
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
<<<<<<< HEAD
uniform sampler2D   brdf_lut;
=======
uniform sampler2D brdf_lut;
>>>>>>> parent of 6e4f8d6 (Update)

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normal_texture, inTexCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(inWorldPos);
    vec3 Q2  = dFdy(inWorldPos);
    vec2 st1 = dFdx(inTexCoord);
    vec2 st2 = dFdy(inTexCoord);

    vec3 N   = normalize(inNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal).xyz;
}

vec3 getNormal()
{
    vec3 n = normalize(inNormal);
    if (object_ubo.has_normal_texture != 0) {
        n = getNormalFromMap();
    }
    return n;
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

vec4 getAlbedo()
{
    if (object_ubo.has_albedo_texture != 0)
        return texture(albedo_texture, inTexCoord);
    return object_ubo.albedo;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.141592 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
<<<<<<< HEAD
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
                * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calculatePointLightReflectance(PointLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic, int lightIndex)
=======
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

vec3 calculatePointLightReflectance(PointLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
>>>>>>> parent of 6e4f8d6 (Update)
{
    vec3 L = normalize(light.position - inWorldPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position - inWorldPos);
    float attenuation = 1.0 / ((1.0 + 0.09 * distance + 0.032 * (distance * distance)) * light.attenuation);
    vec3 radiance = (light.color * vec3(light.power)) * attenuation;

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

vec3 calculateDirLightReflectance(DirLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
{
    float power  = max(light.power, 1.0);
    vec3  color  = max(light.color, vec3(0.0001));

    vec3 L = normalize(light.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = color * power;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
<<<<<<< HEAD
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N,V),0.0) * max(dot(N,L),0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

=======
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
>>>>>>> parent of 6e4f8d6 (Update)
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
	vec4 albedo = getAlbedo();
    vec3 albedo_color = albedo.rgb;
    float albedo_alpha = albedo.a;
	
	if (albedo_alpha < 0.5)
		discard;
	
    float metallic = getMetallic();
    float roughness = clamp(getRoughness(), 0.05, 1.0);
    float ao = getAO();

    vec3 N = getNormal();
    vec3 V = normalize(global_ubo.camera_position - inWorldPos);
    vec3 R = reflect(-V, N); 
	
    vec3 F0 = mix(vec3(0.04), albedo_color, metallic);
<<<<<<< HEAD

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < global_ubo.usePointLight; ++i) {
        Lo += calculatePointLightReflectance(global_ubo.pointLights[i], V, N, F0, albedo_color, roughness, metallic, i);
    }

    for (int i = 0; i < global_ubo.useDirLight; ++i) {
        Lo += calculateDirLightReflectance(global_ubo.dirLights[i], V, N, F0, albedo_color, roughness, metallic, i);
=======
	
	vec3 Lo = vec3(0.0);
    for(int i = 0; i < global_ubo.usePointLight; ++i)
    {
        Lo += calculatePointLightReflectance(global_ubo.pointLights[i], V, N, F0, albedo_color, roughness, metallic);
    }
    for(int i = 0; i < global_ubo.useDirLight; ++i)
    {
        Lo += calculateDirLightReflectance(global_ubo.dirLights[i], V, N, F0, albedo_color, roughness, metallic);
>>>>>>> parent of 6e4f8d6 (Update)
    }
	
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	
	vec3 kS = F; 
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradiance_map, N).rgb;
    vec3 diffuse      = irradiance * albedo_color;

    vec3 prefilteredColor = textureLod(prefilter_map, R,  roughness * global_ubo.prefilterLevels).rgb;    
    vec2 brdf  = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 result = ambient + Lo;

    // HDR tonemapping
    result = result / (result + vec3(1.0));
    // gamma correct
    //result = pow(result, vec3(1.0/2.2)); 

    outColor = vec4(result, 1.0);
    //outColor = vec4(irradiance, 1.0);
}