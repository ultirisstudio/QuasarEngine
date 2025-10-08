#version 450 core

#define MAX_BONES 100

in VS_OUT {
    vec2 uv;
    vec3 worldNormal;
    vec3 worldPos;
} fs_in;

layout(location = 0) out vec4 FragColor;

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

uniform sampler2D albedo_texture;

void main()
{
    vec4 baseColor = (object_ubo.has_albedo_texture == 1)
        ? texture(albedo_texture, fs_in.uv)
        : object_ubo.albedo;

    FragColor = vec4(baseColor.rgb, 1.0);
}
