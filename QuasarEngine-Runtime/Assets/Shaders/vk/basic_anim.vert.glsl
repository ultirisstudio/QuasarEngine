#version 450 core

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in ivec4 inBoneIds;
layout(location = 4) in vec4 inWeights;

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

layout(set = 0, binding = 0) uniform global_uniform_object  {
    mat4 view;
	mat4 projection;
	vec3 camera_position;
	
	int usePointLight;
	int useDirLight;
	
	PointLight pointLights[NR_POINT_LIGHTS];
	DirLight dirLights[NR_DIR_LIGHTS];
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object  {
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
	
	mat4 finalBonesMatrices[MAX_BONES];
} object_ubo;

void main()
{
    vec4 skinnedPos  = vec4(0.0);
    vec3 skinnedNorm = vec3(0.0);

    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        int   id = inBoneIds[i];
        float w  = inWeights[i];
        if (id < 0 || w <= 0.0) continue;

        mat4 B = object_ubo.finalBonesMatrices[id];
        skinnedPos  += (B * vec4(inPosition, 1.0)) * w;
        skinnedNorm += (mat3(B) * inNormal) * w;
    }

    if (skinnedPos == vec4(0.0)) {
        skinnedPos  = vec4(inPosition, 1.0);
        skinnedNorm = inNormal;
    }

    outWorldPos = vec4(object_ubo.model * skinnedPos).xyz;

    outTexCoord = inTexCoord;
	
    vec3 nrm = (skinnedNorm == vec3(0.0)) ? inNormal : skinnedNorm;
	mat3 normalMatrix = transpose(inverse(mat3(object_ubo.model)));
	outNormal = normalize(normalMatrix * normalize(nrm));
	
	gl_Position = global_ubo.projection * global_ubo.view * vec4(outWorldPos, 1.0);
}
