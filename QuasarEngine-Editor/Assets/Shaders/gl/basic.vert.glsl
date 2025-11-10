#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;

struct PointLight {    
    vec3 position;
    vec3 color;
	
    float attenuation;
    float power;
};
#define NR_POINT_LIGHTS 4

struct DirLight {    
    vec3 direction;
	vec3 color;
	
    float power;
};
#define NR_DIR_LIGHTS 4

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
	mat4 projection;
	vec3 camera_position;
	
	int usePointLight;
	int useDirLight;

    int prefilterLevels;
	
	PointLight pointLights[NR_POINT_LIGHTS];
	DirLight dirLights[NR_DIR_LIGHTS];
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

void main()
{
    outTexCoord = inTexCoord;
    mat3 normalMatrix = transpose(inverse(mat3(object_ubo.model)));
    outNormal = normalMatrix * inNormal;
    outWorldPos = vec3(object_ubo.model * vec4(inPosition, 1.0));
    gl_Position = global_ubo.projection * global_ubo.view * vec4(outWorldPos, 1.0);
}