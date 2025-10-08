#version 450 core

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in ivec4 inBoneIds;
layout(location = 4) in vec4  inWeights;

layout(std140, binding = 0) uniform global_uniform_object  {
    mat4 view;
	mat4 projection;
} global_ubo;

layout(std140, binding = 1) uniform local_uniform_object  {
    mat4 model;
    vec4 albedo;
    int has_albedo_texture;
	mat4 finalBonesMatrices[MAX_BONES];
} object_ubo;

out VS_OUT {
    vec2 uv;
    vec3 worldNormal;
    vec3 worldPos;
} vs_out;

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

    vec4 worldPos = object_ubo.model * skinnedPos;
    vs_out.worldPos = worldPos.xyz;

    mat3 normalMat = mat3(transpose(inverse(object_ubo.model)));
    vs_out.worldNormal = normalize(normalMat * normalize(skinnedNorm));

    vs_out.uv = inTexCoord;

    gl_Position = global_ubo.projection * global_ubo.view * worldPos;
}
