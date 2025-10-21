#version 450 core
layout (vertices = 4) out;

struct PointLight { vec3 position; vec3 color; float attenuation; float power; };
struct DirLight   { vec3 direction; vec3 color; float power; };
#define NR_POINT_LIGHTS 4
#define NR_DIR_LIGHTS 4

layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 view;
    mat4 projection;
    vec3 camera_position;
    int  usePointLight;
    int  useDirLight;
    PointLight pointLights[NR_POINT_LIGHTS];
    DirLight   dirLights[NR_DIR_LIGHTS];
} global_ubo;

layout(set = 1, binding = 0) uniform local_uniform_object {
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

layout(location = 0) in  vec2 vTexCoord[];
layout(location = 0) out vec2 tcTexCoord[];

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tcTexCoord[gl_InvocationID] = vTexCoord[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        const int   MIN_TESS_LEVEL = 4;
        const int   MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE   = 20.0;
        const float MAX_DISTANCE   = 800.0;

        vec4 e00 = global_ubo.view * object_ubo.model * gl_in[0].gl_Position;
        vec4 e01 = global_ubo.view * object_ubo.model * gl_in[1].gl_Position;
        vec4 e10 = global_ubo.view * object_ubo.model * gl_in[2].gl_Position;
        vec4 e11 = global_ubo.view * object_ubo.model * gl_in[3].gl_Position;

        float d00 = clamp((abs(e00.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float d01 = clamp((abs(e01.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float d10 = clamp((abs(e10.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);
        float d11 = clamp((abs(e11.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0, 1.0);

        float t0 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL), min(d10, d00));
        float t1 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL), min(d00, d01));
        float t2 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL), min(d01, d11));
        float t3 = mix(float(MAX_TESS_LEVEL), float(MIN_TESS_LEVEL), min(d11, d10));

        gl_TessLevelOuter[0] = t0;
        gl_TessLevelOuter[1] = t1;
        gl_TessLevelOuter[2] = t2;
        gl_TessLevelOuter[3] = t3;

        gl_TessLevelInner[0] = max(t1, t3);
        gl_TessLevelInner[1] = max(t0, t2);
    }
}
